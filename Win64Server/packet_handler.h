#pragma once

#include <winsock2.h>
#include <vector>
#include <cstdint>
#include <map>

#include "Stream.h"
#include "constants.h"
#include "handshake.h"
#include "packet.h"

void process_packet(SOCKET clientSocket, int32_t packetId, const std::vector<char>& packetData);

void handle_client(SOCKET clientSocket);

void handle_recv_error(int received);

size_t receive_data(SOCKET clientSocket, std::vector<char>& packetData, size_t totalReceived, size_t remainingToReceive);

void disconnectUser(SOCKET clientSocket);

void closeClientSocket(SOCKET clientSocket);
