#pragma once

#include <winsock2.h>
#include <cstdint>

#include "basic_stream.h"
#include "packet.h"

class PongPacket : public Packet {
public:
    PongPacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override {};
};

class PilotUpdate : public Packet {
public:
    PilotUpdate(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class ControllerUpdate : public Packet {
public:
    ControllerUpdate(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class TransponderPacket : public Packet {
public:
    TransponderPacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class ChangeCyclePacket : public Packet {
public:
    ChangeCyclePacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class MessageRxPacket : public Packet {
public:
    MessageRxPacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class PilotTitle : public Packet {
public:
    PilotTitle(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class ClientMode : public Packet {
public:
    ClientMode(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class ClientDisconnect : public Packet {
public:
    ClientDisconnect(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class FlightPlanUpdate : public Packet {
public:
    FlightPlanUpdate(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class RequestFlightPlan : public Packet {
public:
    RequestFlightPlan(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class PrivateMsgPacket : public Packet {
public:
    PrivateMsgPacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};

class PrimeFreqPacket : public Packet {
public:
    PrimeFreqPacket(int32_t id, int32_t size) : Packet(id, size) {}

    void handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf) override;
};


extern PongPacket pongPacket;
extern PilotUpdate pilotUpdate;
extern ControllerUpdate controllerUpdate;
extern TransponderPacket transponderPacket;
extern ChangeCyclePacket changeCyclePacket;
extern MessageRxPacket messageRxPacket;
extern PilotTitle pilotTitle;
extern ClientMode clientMode;
extern ClientDisconnect clientDisconnect;
extern FlightPlanUpdate flightPlanUpdate;
extern RequestFlightPlan requestFlightPlan;
extern PrivateMsgPacket privateMsgPacket;
extern PrimeFreqPacket primeFreqPacket;