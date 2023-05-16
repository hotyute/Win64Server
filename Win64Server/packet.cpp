#include "packet.h"

#include "handshake.h"
#include "incoming_packets.h"

PacketFactory packetFactory;

void register_packets()
{
    packetFactory.registerPacket(examplePacket.getPacketId(), &examplePacket);
    packetFactory.registerPacket(pongPacket.getPacketId(), &pongPacket);
    packetFactory.registerPacket(pilotUpdate.getPacketId(), &pilotUpdate);
    packetFactory.registerPacket(controllerUpdate.getPacketId(), &controllerUpdate);
    packetFactory.registerPacket(transponderPacket.getPacketId(), &transponderPacket);
    packetFactory.registerPacket(changeCyclePacket.getPacketId(), &changeCyclePacket);
    packetFactory.registerPacket(messageRxPacket.getPacketId(), &messageRxPacket);
    packetFactory.registerPacket(pilotTitle.getPacketId(), &pilotTitle);
    packetFactory.registerPacket(clientMode.getPacketId(), &clientMode);
    packetFactory.registerPacket(clientDisconnect.getPacketId(), &clientDisconnect);
    packetFactory.registerPacket(flightPlanUpdate.getPacketId(), &flightPlanUpdate);
    packetFactory.registerPacket(requestFlightPlan.getPacketId(), &requestFlightPlan);
    packetFactory.registerPacket(privateMsgPacket.getPacketId(), &privateMsgPacket);
    packetFactory.registerPacket(primeFreqPacket.getPacketId(), &primeFreqPacket);
}