#include "packet_handler.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <map>

#include "client_manager.h"
#include "broadcast_task.h"
#include "basic_stream.h"


extern std::atomic<bool> serverRunning;
extern std::shared_mutex clientSocketsMutex;
extern std::vector<SOCKET> clientSockets;

std::shared_mutex dataMutex;

std::map<SOCKET, std::unique_ptr<BasicStream>> byteData;

void process_packet(SOCKET clientSocket, int32_t packetId, int32_t size, BasicStream& buf) {
	//std::cout << "Received packet ID: " << packetId << ", packet size: " << size << std::endl;
	Packet* packet = packetFactory.getPacket(packetId);
	if (packet != nullptr) {
		packet->handle(clientSocket, clientManager.getUserBySocket(clientSocket), buf);
	}
}

void checkClientData(SOCKET clientSocket) {
	std::unique_lock<std::shared_mutex> lock(dataMutex);
	auto it = byteData.find(clientSocket);
	if (it == byteData.end()) {
		byteData.emplace(clientSocket, std::make_unique<BasicStream>());
	}
}

void removeClientData(SOCKET clientSocket) {
	std::unique_lock<std::shared_mutex> lock(dataMutex);
	byteData.erase(clientSocket);
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
			packet->handle(clientSocket, clientManager.getUserBySocket(clientSocket), buf);
		}
	}
};


void handle_client(SOCKET clientSocket) {
	// Initialize the Stream buffer for the clientSocket if not already created
	checkClientData(clientSocket);

	BasicStream& buf = *byteData[clientSocket];

	// Check for available bytes in the socket
	u_long bytes_available = 0;
	int result = ioctlsocket(clientSocket, FIONREAD, &bytes_available);

	// Read data from the socket into the buffer
	buf.add_data(clientSocket);

	// Process packets in the buffer
	PacketProcessor::processStream(clientSocket, buf);
}

void handle_recv_error(int received) {
	if (received == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK) {
			std::cerr << "Error receiving data: " << error << std::endl;
		}
	}
	else if (received == 0) {
		std::cout << "Client disconnected" << std::endl;
	}
	else {
		std::cerr << "Received incomplete header" << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(30));
}

void disconnectUser(SOCKET clientSocket) {
	// Perform user-specific cleanup, such as saving user data, notifying other users, etc.
	// Example:
	std::shared_ptr<User> user = clientManager.getUserBySocket(clientSocket);
	if (user && user->isValid()) {
		// Save user data, remove from chat rooms, etc.
		// ...
		// Remove the user from the user manager:
		clientManager.removeUser(clientSocket);
	}

	// Close the client socket and remove it from the clientSockets vector
	closeClientSocket(clientSocket);
}

void closeClientSocket(SOCKET clientSocket) {
	removeClientData(clientSocket);

	std::shared_lock<std::shared_mutex> lock(clientSocketsMutex);
	auto it = std::find(clientSockets.begin(), clientSockets.end(), clientSocket);
	if (it != clientSockets.end()) {
		clientSockets.erase(it);
		closesocket(clientSocket);
	}
}







