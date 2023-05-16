#pragma once

#include <winsock2.h>
#include <cstdint>

#include "packet.h"

class HandshakePacket : public Packet {
public:
    HandshakePacket(int32_t id, int32_t size)
        : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

extern HandshakePacket examplePacket;