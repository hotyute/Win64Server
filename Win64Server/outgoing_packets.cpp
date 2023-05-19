#include "outgoing_packets.h"

#include <iostream>

#include "Stream.h"
#include "tools.h"
#include "aircraft.h"
#include "controller.h"

void send_server_message(User& user, const char* message) {
	auto out = Stream(256);
	out.createFrameVarSize(7);
	out.writeString(message);
	out.endFrameVarSize();
	write(user, out);
}

void send_wx(User& user, const char* weather) {
	auto out = Stream(128);
	out.createFrameVarSize(8);
	out.writeString(weather);
	out.endFrameVarSize();
	write(user, out);
}

void create_client(User& user, User& client)
{
	auto out = Stream(256);
	const int clientType = client.getType();
	out.createFrameVarSizeWord(9);
	out.writeWord(client.getIndex());
	out.writeByte(clientType);
	const Identity id = client.getIdentity();
	out.writeString((id.callsign.c_str()));
	out.writeString((id.username.c_str()));
	out.writeString((id.login_name.c_str()));
	out.writeWord(client.getVisibilityRange());
	out.writeQWord(doubleToRawBits(client.getLocation().getLatitude()));
	out.writeQWord(doubleToRawBits(client.getLocation().getLongitude()));
	if (clientType == AV_CLIENT::CONTROLLER)
	{
		const auto& controller = dynamic_cast<Controller&>(client);
		out.writeByte(controller.controller_rating);
		out.writeByte(controller.controller_position);
		out.write3Byte(controller.frequencies[0]);
	}
	else if (clientType == AV_CLIENT::PILOT)
	{
		auto& aircraft = dynamic_cast<Aircraft&>(client);
		out.writeString(aircraft.getAcfTitle().c_str());
		out.writeString(aircraft.getTransponder().c_str());
		out.writeByte(aircraft.getMode() << 4 | (aircraft.heavy ? 1 : 0));
		out.writeQWord(((static_cast<int>((aircraft.getState().getPitch() * 1024.0) / -360.0) << 22)
			+ (static_cast<int>((aircraft.getState().getRoll() * 1024.0) / -360.0) << 12)
			+ (static_cast<int>((aircraft.getState().getHeading() * 1024.0) / 360.0) << 2)));
	}
	out.endFrameVarSizeWord();
	write(user, out);
}

void create_flight_plan(User& user, User& client) {
	if (client.getType() == AV_CLIENT::PILOT) {
		const FlightPlan* fp_ptr = dynamic_cast<Aircraft&>(client).getFlightPlan();
		if (fp_ptr && fp_ptr->cycle) {
			send_flight_plan_cycle(user, client);
		}
	}
}

void send_time_change(User& user, const long long time) {
	auto out = Stream(10);
	out.createFrame(10);
	out.writeQWord(time);
	printf("send_time: %s %lld\n", user.getCallsign().c_str(), time);
	write(user, out);
}

void send_private_message(User& user, const std::string& from, const char* message) {
	auto out = Stream(256);
	out.createFrameVarSizeWord(11);
	out.writeString(from.c_str());
	out.writeString(message);
	out.endFrameVarSizeWord();
	write(user, out);
}

void delete_client(User& user, const User& client) {
	auto out = Stream(3);
	out.createFrame(12);
	out.writeWord(client.getIndex());
	write(user, out);
}

void send_ping(User& user) {
	auto out = Stream(1);
	out.createFrame(13);
	write(user, out);
}

void send_pilot_update(User& user, Aircraft& other) {
	auto out = Stream(37);
	out.createFrame(14);
	out.writeWord(other.getIndex());
	const double latitude = other.getLocation().getLatitude();
	const double longitude = other.getLocation().getLongitude();
	out.writeQWord(doubleToRawBits(latitude));
	out.writeQWord(doubleToRawBits(longitude));
	const long long infoHash = (static_cast<int>((other.getState().getPitch() * 1024.0) / -360.0) << 22)
		+ (static_cast<int>((other.getState().getRoll() * 1024.0) / -360.0) << 12)
		+ (static_cast<int>((other.getState().getHeading() * 1024.0) / 360.0) << 2);
	out.writeQWord(infoHash);
	out.writeWord(other.getState().getGroundSpeed());
	const double altitude = other.getState().getAltitude();
	out.writeQWord(doubleToRawBits(altitude));
	write(user, out);
}

void send_message(User& user, const User& from, const int frequency, const char* message, const bool asel) {
	auto out = Stream(512);
	out.createFrameVarSizeWord(15);
	out.writeWord(from.getIndex());
	out.write3Byte(frequency);
	out.writeByte(asel ? 1 : 0);
	out.writeString(message);
	out.endFrameVarSizeWord();
	write(user, out);
}

void send_client_mode_change(User& user, const Aircraft& other) {
	auto out = Stream(4);
	out.createFrame(16);
	out.writeWord(other.getIndex());
	out.writeByte(other.getMode());
	write(user, out);
}

void send_flight_plan_cycle(User& user, User& from) {
	auto out = Stream(256);
	const FlightPlan& fp = *dynamic_cast<Aircraft&>(from).getFlightPlan();
	out.createFrameVarSizeWord(17);
	out.writeWord(from.getIndex());
	out.writeWord(fp.cycle);
	out.writeByte(from.getType());
	if (from.getType() == PILOT_CLIENT)
	{
		out.writeByte(fp.flightRules);
		out.writeString(fp.squawkCode.c_str());
		out.writeString(fp.departure.c_str());
		out.writeString(fp.arrival.c_str());
		out.writeString(fp.alternate.c_str());
		out.writeString(fp.cruise.c_str());
		out.writeString(fp.acType.c_str());
		out.writeString(fp.scratchPad.c_str());
		out.writeString(fp.route.c_str());
		out.writeString(fp.remarks.c_str());
	}
	out.endFrameVarSizeWord();
	write(user, out);
}

void send_controller_update(User& user, Controller& other) {
	auto out = Stream(20);
	out.createFrame(18);
	out.writeWord(other.getIndex());
	const double latitude = other.getLocation().getLatitude();
	const double longitude = other.getLocation().getLongitude();
	out.writeQWord(doubleToRawBits(latitude));
	out.writeQWord(doubleToRawBits(longitude));
	out.writeByte(0);
	write(user, out);
}

void send_vis_update(User& user, const User& update) {
	auto out = Stream(5);
	out.createFrame(19);
	out.writeWord(update.getIndex());
	out.writeWord(update.getVisibilityRange());
	write(user, out);
}

void send_client_transponder(User& user, const Aircraft& other) {
	auto out = Stream(20);
	out.createFrameVarSize(20);
	out.writeWord(other.getIndex());
	out.writeString(other.getTransponder().c_str());
	out.endFrameVarSize();
	write(user, out);
}

void send_prim_freq(User& user, const User& other)
{
	auto out = Stream(8);
	out.createFrame(21);
	out.writeWord(other.getIndex());
	out.writeByte(0);
	out.writeDWord(other.frequencies[0]);
	write(user, out);
}

void write(User& user, const Stream& stream) {
	std::lock_guard<std::mutex> lock(user.send_data_mutex);
	send_data(user.getClientSocket(), std::vector<char>(stream.buffer, stream.buffer + stream.currentOffset));
}

void send_data(SOCKET clientSocket, const std::vector<char>& buffer) {
	size_t total_sent = 0;
	size_t remaining = buffer.size();

	while (total_sent < buffer.size()) {
		const int sent = send(clientSocket, buffer.data() + total_sent, static_cast<int>(remaining), 0);
		if (sent == SOCKET_ERROR) {
			std::cerr << "Error sending data: " << WSAGetLastError() << std::endl;
			break;
		}

		total_sent += sent;
		remaining -= sent;
	}
}
