#include "packet.h"
#include "packet_handler.h"
#include "thread_pool.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <winsock2.h>

#include "broadcast_task.h"
#include "client_updater.h"

#pragma comment(lib, "Ws2_32.lib")

constexpr int PORT = 4403;
constexpr int NUM_THREADS = 8;
constexpr int PROCESS_THREADS = 2;
constexpr int TIMER_INTERVAL = 30;

std::atomic<bool> serverRunning;
std::shared_mutex clientSocketsMutex;
std::vector<SOCKET> clientSockets;
std::shared_mutex lastPacketReceivedMutex;
std::unordered_map<SOCKET, std::chrono::steady_clock::time_point> lastPacketReceived;

int init_winsock(WSADATA& wsData) {
	WORD ver = MAKEWORD(2, 2);
	return WSAStartup(ver, &wsData);
}

SOCKET createSocket() {
	return socket(AF_INET, SOCK_STREAM, 0);
}

bool bindSocket(const SOCKET& listening) {
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	hint.sin_addr.S_un.S_addr = INADDR_ANY;

	return bind(listening, (sockaddr*)&hint, sizeof(hint)) != SOCKET_ERROR;
}

bool setSocketToListen(const SOCKET& listening) {
	return listen(listening, SOMAXCONN) != SOCKET_ERROR;
}

void process_clients(const std::vector<SOCKET>& client_sockets, std::atomic<bool>& running, size_t start, size_t end) {
	std::shared_lock<std::shared_mutex> lock(clientSocketsMutex);
	for (size_t i = start; i < end; ++i) {
		const SOCKET clientSocket = client_sockets[i];
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(clientSocket, &readfds);

		timeval timeout{};
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		const int activity = select(0, &readfds, nullptr, nullptr, &timeout);
		if (activity == SOCKET_ERROR) {
			const int error = WSAGetLastError();
			std::cerr << "Select failed! Error: " << error << std::endl;
			continue;
		}

		if (activity > 0 && FD_ISSET(clientSocket, &readfds)) {
			handle_client(clientSocket);
		}
	}
}

void timerFunction(const std::atomic<bool>& server_running, ThreadPool& threadPool) {
	constexpr std::chrono::milliseconds timerCvInterval(30);    // Adjust this value for the desired interval of the timerCv timer
	constexpr std::chrono::seconds timeoutDuration(11); // Timeout after 11 seconds of inactivity

	auto last_timed_task_execution = std::vector<std::chrono::steady_clock::time_point>(TimedTask::get_tasks().size(), std::chrono::steady_clock::now());
	auto last_timer_cv_execution = std::chrono::steady_clock::now();

	while (server_running) {
		auto currentTime = std::chrono::steady_clock::now();

		// Enqueue tasks if the interval has passed
		{
			std::unique_lock<std::mutex> lock(TimedTask::get_tasks_mutex());
			for (size_t i = 0; i < TimedTask::get_tasks().size(); ++i) {
				TimedTask* task = TimedTask::get_tasks()[i];
				if (task->is_running() && currentTime - last_timed_task_execution[i] >= task->get_duration()) {
					task->enqueue_execute();
					last_timed_task_execution[i] = currentTime;
				}
			}
		}

		// Check for timeouts
		{
			std::shared_lock<std::shared_mutex> lock(lastPacketReceivedMutex);
			auto current_time = std::chrono::steady_clock::now();

			for (auto it = lastPacketReceived.begin(); it != lastPacketReceived.end(); /* no increment here */) {
				if (current_time - it->second > timeoutDuration) {
					// Handle socket timeout
					threadPool.enqueue([&it]() {
						handle_timeout(it->first);
						});

					// Remove it from the lastPacketReceived map and increment the iterator
					it = lastPacketReceived.erase(it);
				}
				else {
					++it;
				}
			}
		}

		// Check if the clientSockets interval has passed
		if (currentTime - last_timer_cv_execution >= timerCvInterval) {
			const size_t num_clients = clientSockets.size();
			const size_t clients_per_thread = num_clients / PROCESS_THREADS;
			size_t remaining_clients = num_clients % PROCESS_THREADS;

			size_t start = 0;
			size_t end = 0;

			for (int i = 0; i < PROCESS_THREADS; i++) {
				start = end;
				end = start + clients_per_thread;

				if (remaining_clients > 0) {
					++end;
					--remaining_clients;
				}

				threadPool.enqueue([&, start, end]() {
					process_clients(clientSockets, serverRunning, start, end);
					});
			}

			last_timer_cv_execution = currentTime;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Sleep for a short time to avoid busy looping
	}
}


int main() {

	// Register your packet instances with their IDs
	register_packets();


	WSADATA wsData;
	if (init_winsock(wsData) != 0) {
		std::cerr << "Can't initialize Winsock! Quitting" << std::endl;
		return 1;
	}

	SOCKET listening = createSocket();
	if (listening == INVALID_SOCKET) {
		std::cerr << "Can't create a socket! Quitting" << std::endl;
		WSACleanup();
		return 1;
	}

	if (!bindSocket(listening)) {
		std::cerr << "Can't bind socket! Quitting" << std::endl;
		closesocket(listening);
		WSACleanup();
		return 1;
	}

	if (!setSocketToListen(listening)) {
		std::cerr << "Can't set socket to listen! Quitting" << std::endl;
		closesocket(listening);
		WSACleanup();
		return 1;
	}

	u_long mode = 1;
	if (ioctlsocket(listening, FIONBIO, &mode) == SOCKET_ERROR) {
		std::cerr << "Can't set socket to non-blocking! Quitting" << std::endl;
		closesocket(listening);
		WSACleanup();
		return 1;
	}

	// ... Bind the socket and start listening ...

	std::cout << "TCP server is listening on port " << PORT << std::endl;

	// Initialize the thread pool
	ThreadPool threadPool(NUM_THREADS);

	// Timer thread
	std::thread timerThread([&]() {
		timerFunction(serverRunning, threadPool);
		});

	constexpr std::chrono::milliseconds broadcast_interval(10000);
	// Create a Tasks
	constexpr std::chrono::milliseconds update_interval(50); // Broadcast every 10 seconds PING Pakcet
	BroadcastTask broadcast_task(threadPool, broadcast_interval);
	UpdateTask update_task(threadPool, update_interval);

	// Start the Tasks
	broadcast_task.start();
	update_task.start();

	// Main server loop
	serverRunning = true;
	while (serverRunning) {
		sockaddr_in client;
		int clientSize = sizeof(client);

		SOCKET clientSocket = accept(listening, (sockaddr*)&client, &clientSize);
		if (clientSocket == INVALID_SOCKET) {
			int error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK) {
				std::cerr << "Client connection failed! Error: " << error << std::endl;
			}

			// Sleep for a short duration to avoid high CPU usage
			std::this_thread::sleep_for(std::chrono::milliseconds(30));
			continue;
		}

		// Set the client socket to non-blocking mode
		u_long mode = 1;
		if (ioctlsocket(clientSocket, FIONBIO, &mode) == SOCKET_ERROR) {
			std::cerr << "Can't set client socket to non-blocking! Error: " << WSAGetLastError() << std::endl;
			closesocket(clientSocket);
			continue;
		}

		{
			std::unique_lock<std::shared_mutex> lock(clientSocketsMutex);
			clientSockets.push_back(clientSocket);
		}
	}

	// ... Cleanup ...
	for (const SOCKET clientSocket : clientSockets) {
		closesocket(clientSocket);
	}

	closesocket(listening);
	WSACleanup();

	timerThread.join();

	return 0;
}
