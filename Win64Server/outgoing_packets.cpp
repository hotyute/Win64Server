#include "outgoing_packets.h"

#include <iostream>

#include "Stream.h"
#include "tools.h"
#include "aircraft.h"
#include "controller.h"

void send_server_message(User& user, const char* message) {
	auto out = BasicStream(256);
	out.create_frame_var_size(7);
	out.write_string(message);
	out.end_frame_var_size();
	write(user, out);
}

void send_wx(User& user, const char* weather) {
	auto out = BasicStream(128);
	out.create_frame_var_size(8);
	out.write_string(weather);
	out.end_frame_var_size();
	write(user, out);
}

void create_client(User& user, User& client)
{
	auto out = BasicStream(256);
	const int clientType = client.getType();
	out.create_frame_var_size_word(9);
	out.write_short(client.getIndex());
	out.write_byte(clientType);
	const Identity id = client.getIdentity();
	out.write_string((id.callsign.c_str()));
	out.write_string((id.username.c_str()));
	out.write_string((id.login_name.c_str()));
	out.write_short(client.getVisibilityRange());
	out.write_qword(doubleToRawBits(client.getLocation().getLatitude()));
	out.write_qword(doubleToRawBits(client.getLocation().getLongitude()));
	if (clientType == AV_CLIENT::CONTROLLER)
	{
		const auto& controller = dynamic_cast<Controller&>(client);
		out.write_byte(controller.controller_rating);
		out.write_byte(controller.controller_position);
		out.write_3byte(controller.frequencies[0]);
	}
	else if (clientType == AV_CLIENT::PILOT)
	{
		auto& aircraft = dynamic_cast<Aircraft&>(client);
		out.write_string(aircraft.getAcfTitle().c_str());
		out.write_string(aircraft.getTransponder().c_str());
		out.write_byte(aircraft.getMode() << 4 | (aircraft.heavy ? 1 : 0));
		out.write_qword(((static_cast<int>((aircraft.getState().getPitch() * 1024.0) / -360.0) << 22)
			+ (static_cast<int>((aircraft.getState().getRoll() * 1024.0) / -360.0) << 12)
			+ (static_cast<int>((aircraft.getState().getHeading() * 1024.0) / 360.0) << 2)));
	}
	out.end_frame_var_size_word();
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
	auto out = BasicStream(10);
	out.create_frame(10);
	out.write_qword(time);
	printf("send_time: %s %lld\n", user.getCallsign().c_str(), time);
	write(user, out);
}

void send_private_message(User& user, const std::string& from, const char* message) {
	auto out = BasicStream(256);
	out.create_frame_var_size_word(11);
	out.write_string(from.c_str());
	out.write_string(message);
	out.end_frame_var_size_word();
	write(user, out);
}

void delete_client(User& user, const User& client) {
	auto out = BasicStream(3);
	out.create_frame(12);
	out.write_short(client.getIndex());
	write(user, out);
}

void send_ping(User& user) {
	auto out = BasicStream(1);
	out.create_frame(13);
	write(user, out);
}

void send_pilot_update(User& user, Aircraft& other) {
	auto out = BasicStream(37);
	out.create_frame(14);
	out.write_short(other.getIndex());
	const double latitude = other.getLocation().getLatitude();
	const double longitude = other.getLocation().getLongitude();
	out.write_qword(doubleToRawBits(latitude));
	out.write_qword(doubleToRawBits(longitude));
	const long long infoHash = (static_cast<int>((other.getState().getPitch() * 1024.0) / -360.0) << 22)
		+ (static_cast<int>((other.getState().getRoll() * 1024.0) / -360.0) << 12)
		+ (static_cast<int>((other.getState().getHeading() * 1024.0) / 360.0) << 2);
	out.write_qword(infoHash);
	out.write_short(other.getState().getGroundSpeed());
	const double altitude = other.getState().getAltitude();
	out.write_qword(doubleToRawBits(altitude));
	write(user, out);
}

void send_message(User& user, const User& from, const int frequency, const char* message, const bool asel) {
	auto out = BasicStream(512);
	out.create_frame_var_size_word(15);
	out.write_short(from.getIndex());
	out.write_3byte(frequency);
	out.write_byte(asel ? 1 : 0);
	out.write_string(message);
	out.end_frame_var_size_word();
	write(user, out);
}

void send_client_mode_change(User& user, const Aircraft& other) {
	auto out = BasicStream(4);
	out.create_frame(16);
	out.write_short(other.getIndex());
	out.write_byte(other.getMode());
	write(user, out);
}

void send_flight_plan_cycle(User& user, User& from) {
	auto out = BasicStream(256);
	const FlightPlan& fp = *dynamic_cast<Aircraft&>(from).getFlightPlan();
	out.create_frame_var_size_word(17);
	out.write_short(from.getIndex());
	out.write_short(fp.cycle);
	out.write_byte(from.getType());
	if (from.getType() == PILOT_CLIENT)
	{
		out.write_byte(fp.flightRules);
		out.write_string(fp.squawkCode.c_str());
		out.write_string(fp.departure.c_str());
		out.write_string(fp.arrival.c_str());
		out.write_string(fp.alternate.c_str());
		out.write_string(fp.cruise.c_str());
		out.write_string(fp.acType.c_str());
		out.write_string(fp.scratchPad.c_str());
		out.write_string(fp.route.c_str());
		out.write_string(fp.remarks.c_str());
	}
	out.end_frame_var_size_word();
	write(user, out);
}

void send_controller_update(User& user, Controller& other) {
	auto out = BasicStream(20);
	out.create_frame(18);
	out.write_short(other.getIndex());
	const double latitude = other.getLocation().getLatitude();
	const double longitude = other.getLocation().getLongitude();
	out.write_qword(doubleToRawBits(latitude));
	out.write_qword(doubleToRawBits(longitude));
	out.write_byte(0);
	write(user, out);
}

void send_vis_update(User& user, const User& update) {
	auto out = BasicStream(5);
	out.create_frame(19);
	out.write_short(update.getIndex());
	out.write_short(update.getVisibilityRange());
	write(user, out);
}

void send_client_transponder(User& user, const Aircraft& other) {
	auto out = BasicStream(20);
	out.create_frame_var_size(20);
	out.write_short(other.getIndex());
	out.write_string(other.getTransponder().c_str());
	out.end_frame_var_size();
	write(user, out);
}

void send_prim_freq(User& user, const User& other)
{
	auto out = BasicStream(8);
	out.create_frame(21);
	out.write_short(other.getIndex());
	out.write_byte(0);
	out.write_int(other.frequencies[0]);
	write(user, out);
}

void send_script(User& user, const User& other, ClientScript& script) {
	auto out = BasicStream(20);
	out.create_frame_var_size_word(22);
	out.write_short(other.getIndex());
	out.write_short(script.idx);
	out.write_string(script.assembly.c_str());
	for (int i_11_ = script.assembly.length() - 1; i_11_ >= 0; i_11_--)
	{
		if (script.assembly.at(i_11_) == 's')
			out.write_string(std::any_cast<std::string>(script.objects[i_11_ + 1]).c_str());
		else if (script.assembly.at(i_11_) == 'l')
			out.write_qword(std::any_cast<long long>(script.objects[i_11_ + 1]));
		else
			out.write_int(std::any_cast<int>(script.objects[i_11_ + 1]));
	}
	out.write_int(std::any_cast<int>(script.objects[0]));
	out.end_frame_var_size_word();
	write(user, out);
}

void write(User& user, BasicStream& stream) {
	std::lock_guard<std::mutex> lock(user.send_data_mutex);
	if (stream.get_index() == 0) {
		printf("Can't flush empty stream o.O");
		return;
	}
	send_data(user.getClientSocket(), std::vector<char>(stream.data, stream.data + stream.index));
	stream.clear();
}

void send_data(SOCKET clientSocket, const std::vector<char>& buffer) {
	size_t total_sent = 0;
	size_t remaining = buffer.size();

	while (total_sent < buffer.size()) {
		const int sent = send(clientSocket, buffer.data() + total_sent, static_cast<int>(remaining), 0);
		if (sent == SOCKET_ERROR) {
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK) {
				// Sleep for a short period of time to prevent busy waiting
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			std::cerr << "Error sending data: " << error << std::endl;
			break;
		}

		total_sent += sent;
		remaining -= sent;
	}
}
