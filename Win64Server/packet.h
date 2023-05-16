#pragma once

#include <winsock2.h>
#include <cstdint>
#include <map>

#include "basic_stream.h"
#include "outgoing_packets.h"
#include "user.h"

class Packet;

class PacketFactory {
public:
	void registerPacket(int32_t id, Packet* packet) {
		packetMap[id] = packet;
	}

	Packet* getPacket(int32_t id) {
		if (packetMap.find(id) != packetMap.end()) {
			return packetMap[id];
		}
		return nullptr;
	}

private:
	std::map<int32_t, Packet*> packetMap;
};

class Packet {
public:
	Packet(int32_t id, int32_t si)
		: packetId(id), size(si) {}

	virtual ~Packet() = default;

	int32_t getPacketId() const {
		return packetId;
	}

	int32_t getSize() const {
		return size;
	}

	// Virtual function to be overridden by derived classes
	virtual void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) = 0;

private:
	int32_t packetId;
	int32_t size;
};

extern PacketFactory packetFactory;

void register_packets();