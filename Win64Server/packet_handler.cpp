#include "packet_handler.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <map>

#include "client_manager.h"
#include "broadcast_task.h"
#include "basic_stream.h"
#include "packet.h"


extern std::atomic<bool> serverRunning;

std::shared_mutex data_mutex;

std::map<SOCKET, std::shared_ptr<BasicStream>> byte_data;

void process_packet(SOCKET clientSocket, int32_t packetId, int32_t size, BasicStream& buf) {
	//std::cout << "Received packet ID: " << packetId << ", packet size: " << size << std::endl;
	Packet* packet = packetFactory.getPacket(packetId);
	if (packet != nullptr) {
		packet->handle(clientSocket, clientManager.get_user_by_socket(clientSocket), buf);
	}
}

void checkClientData(SOCKET clientSocket) {
	std::unique_lock<std::shared_mutex> lock(data_mutex);
	auto it = byte_data.find(clientSocket);
	if (it == byte_data.end()) {
		byte_data.emplace(clientSocket, std::make_shared<BasicStream>());
	}
}

void removeClientData(SOCKET clientSocket) {
	std::unique_lock<std::shared_mutex> lock(data_mutex);
	byte_data.erase(clientSocket);
}

class PacketProcessor {
public:
	static void processStream(SOCKET clientSocket, BasicStream& stream) {
		while (stream.available() > 0) {
			stream.mark_position();

			if (stream.available() < 1) {
				// Not enough data for packet ID, wait for more data
				stream.reset();
				break;
			}

			const uint8_t packet_id = stream.read_unsigned_byte();

			const Packet* packet = packetFactory.getPacket(packet_id);

			int32_t packet_size = packet->getSize();

			if (packet_size == -1) {
				if (stream.available() < 1) {
					// Not enough data for packet size, wait for more data
					stream.reset();
					break;
				}
				packet_size = stream.read_unsigned_byte();
			}
			else if (packet_size == -2) {
				if (stream.available() < 2) {
					// Not enough data for packet size, wait for more data
					stream.reset();
					break;
				}
				packet_size = stream.read_unsigned_short();
			}

			if (packet_size < 0) {
				std::cerr << "Invalid packet size" << std::endl;
				stream.reset();
				break;
			}

			if (stream.available() < static_cast<std::size_t>(packet_size)) {
				// Not enough data for the packet, wait for more data
				stream.reset();
				break;
			}

			// Process the packet
			process_packet(clientSocket, packet_id, packet_size, stream);

			// Delete the processed packet header data from the stream
			stream.delete_marked_block();
		}
	}

private:
	static void process_packet(SOCKET clientSocket, int32_t packetId, int32_t size, BasicStream& buf) {
		//std::cout << "Received packet ID: " << packetId << ", packet size: " << size << std::endl;
		Packet* packet = packetFactory.getPacket(packetId);
		if (packet != nullptr) {
			packet->handle(clientSocket, clientManager.get_user_by_socket(clientSocket), buf);
		}
	}
};


void handle_client(SOCKET clientSocket) {
	// Initialize the Stream buffer for the clientSocket if not already created
	checkClientData(clientSocket);

	std::shared_ptr<BasicStream> buf_ptr;
	{
		std::shared_lock<std::shared_mutex> lock(data_mutex);
		buf_ptr = byte_data[clientSocket];
	}
	BasicStream& buf = *buf_ptr;

	// Check for available bytes in the socket
	u_long bytes_available = 0;
	int result = ioctlsocket(clientSocket, FIONREAD, &bytes_available);

	{
		std::unique_lock<std::shared_mutex> lock(lastPacketReceivedMutex);
		lastPacketReceived[clientSocket] = std::chrono::steady_clock::now();
	}

	// Read data from the socket into the buffer
	const int recv_bytes = buf.add_data(clientSocket);

	check_recv_error(clientSocket, recv_bytes);

	// Process packets in the buffer
	PacketProcessor::processStream(clientSocket, buf);
}

void check_recv_error(SOCKET clientSocket, int received) {
	if (received == SOCKET_ERROR) {
		const int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK) {
			std::cerr << "Error receiving data: " << error << std::endl;
		}
	}
	else if (received == 0) {
		std::cout << "Client disconnected" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
	}
}

void disconnect_user(SOCKET clientSocket) {
	// Perform user-specific cleanup, such as saving user data, notifying other users, etc.
	// Example:
	const std::shared_ptr<User> user = clientManager.get_user_by_socket(clientSocket);
	if (user && user->isValid()) {
		// Save user data, remove from chat rooms, etc.
		user->iterateLocalUsers([&](const auto& local) {
			User& other = *local.first;
			delete_client(other, *user);
			{
				std::unique_lock<std::shared_mutex> lock(other.local_mutex);
				other.local_users.erase(user);
			}
			adjust_time(local.first);
			});
		// Remove the user from the user manager:
		clientManager.remove_user(clientSocket);
	}
}

void removeClientSocket(SOCKET clientSocket, bool close) {
	removeClientData(clientSocket);

	std::shared_lock<std::shared_mutex> lock(clientSocketsMutex);
	const auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
	if (it != clientSockets.end()) {
		clientSockets.erase(it);
		if (close) {
			closesocket(clientSocket);
		}
	}
}

void handle_timeout(SOCKET clientSocket, bool close) {

}





