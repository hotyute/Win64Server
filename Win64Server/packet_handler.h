#pragma once

#include <winsock2.h>
#include <vector>
#include <cstdint>

void process_packet(SOCKET clientSocket, int32_t packetId, const std::vector<char>& packetData);

void handle_client(SOCKET clientSocket);

void check_recv_error(SOCKET clientSocket, int received);

size_t receive_data(SOCKET clientSocket, std::vector<char>& packetData, size_t totalReceived, size_t remainingToReceive);

void disconnect_user(SOCKET clientSocket);

void removeClientSocket(SOCKET clientSocket, bool close = false);

void handle_timeout(SOCKET clientSocket, bool close = false);
