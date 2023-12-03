#include "incoming_packets.h"

#include <iostream>
#include <any>

#include "client_script.h"
#include "basic_stream.h"
#include "boost/date_time/posix_time/posix_time.hpp"

#include "tools.h"
#include "client_manager.h"

PilotUpdate pilotUpdate(1, -1);
TransponderPacket transponderPacket(2, -1);
ControllerUpdate controllerUpdate(3, -1);
PongPacket pongPacket(4, 0);
ChangeCyclePacket changeCyclePacket(5, 11);
MessageRxPacket messageRxPacket(6, -2);
PilotTitle pilotTitle(7, -1);
ClientMode clientMode(8, 3);
ClientDisconnect clientDisconnect(9, 1);
FlightPlanUpdate flightPlanUpdate(10, -2);
RequestFlightPlan requestFlightPlan(11, 4);
PrivateMsgPacket privateMsgPacket(12, -2);
PrimeFreqPacket primeFreqPacket(13, 7);
TempDataPacket tempDataPacket(14, -2);
TempDataRemovePacket tempDataRemovePacket(15, 4);

void PilotUpdate::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const double latitude = longToRawBits(buf.readQWord());
	const double longitude = longToRawBits(buf.readQWord());
	user->getLocation().setLatitude(latitude);
	user->getLocation().setLongitude(longitude);
	const long long hash = buf.readQWord();
	const int num2 = static_cast<int>(hash >> 22);
	const int num3 = static_cast<int>(hash >> 12 & 0x3ff);
	const int num4 = static_cast<int>(hash >> 2 & 0x3ff);
	const double pitch = num2 / 1024.0 * -360.0;
	const double roll = num3 / 1024.0 * -360.0;
	const double heading = num4 / 1024.0 * 360.0;
	const int ground_speed = buf.read_unsigned_short();
	const double altitude = longToRawBits(buf.readQWord());
	if (user->getType() == PILOT) {
		auto& aircraft = dynamic_cast<Aircraft&>(*user);
		aircraft.getState().setPitch(pitch);
		aircraft.getState().setRoll(roll);
		aircraft.getState().setHeading(heading);
		aircraft.getState().setGroundSpeed(ground_speed);
		aircraft.getState().setAltitude(altitude);
	}
	user->setLastPositionUpdate(boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds());
}

void ControllerUpdate::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const double latitude = longToRawBits(buf.readQWord());
	const double longitude = longToRawBits(buf.readQWord());
	user->getLocation().setLatitude(latitude);
	user->getLocation().setLongitude(longitude);
	user->setLastPositionUpdate(boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds());
}

void TransponderPacket::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const std::string code = buf.read_string();
	if (user->getType() == PILOT) {
		auto& aircraft = dynamic_cast<Aircraft&>(*user);
		aircraft.setTransponder(code);
		user->iterateLocalUsers([&user, &aircraft](const auto& local) {
			User& other = *local.first;
			if (other.getType() == CONTROLLER) {
				send_client_transponder(other, aircraft);
			}
			});
	}
}

void ChangeCyclePacket::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	const int index = buf.read_unsigned_short();
	const std::shared_ptr<User> user1 = clientManager.get_user_by_index(index);
	const int type = buf.read_unsigned_byte();
	const long long request_time = buf.readQWord();
	if (user1 == user) {
		user->setRequestedInterval(type, request_time);
	}
}

void MessageRxPacket::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const std::string callsign = buf.read_string();
	const int frequency = static_cast<int>(buf.read3Byte());
	const std::string message = buf.read_string();
	const std::shared_ptr<User> asel = clientManager.get_user_by_callsign(callsign);
	user->iterateLocalUsers([&](const auto& local) {
		User& users = *local.first;
		for (const auto& comm : users.frequencies) {
			if (comm != 99998 && comm == frequency) {
				const bool is_asel = (asel ? users.getCallsign() == asel->getCallsign() : false);
				send_message(users, *user, frequency, message.c_str(), is_asel);
				break;
			}
		}
		});
}

void PilotTitle::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	const int index = buf.read_unsigned_short();
	const std::shared_ptr<User> user1 = clientManager.get_user_by_index(index);
	const std::string title = buf.read_string();
	if (user1 == user && user1->getType() == PILOT_CLIENT) {
		auto& aircraft = dynamic_cast<Aircraft&>(*user);
		aircraft.setAcfTitle(title);
	}
}

void ClientMode::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const int i = buf.read_unsigned_byte();
	const int mode = i >> 4, heavy = i & 0xF;
	if (user->getType() == PILOT) {
		auto& aircraft = dynamic_cast<Aircraft&>(*user);
		aircraft.setMode(mode);
		aircraft.setHeavy(heavy);

		//TODO move this somewhere else
		user->iterateLocalUsers([&user, &aircraft](const auto& local) {
			send_client_mode_change(*local.first, aircraft);
			});
	}
}

void ClientDisconnect::handle(const SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	int flag = buf.read_unsigned_byte();
	User& _user = *user;
	closesocket(client_socket);
	printf("[--User: %s has disconnected--]\n", user->getIdentity().username.c_str());
}

void FlightPlanUpdate::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	int cur_cycle = buf.read_unsigned_short();
	int index = buf.read_unsigned_short();
	std::shared_ptr<User> user1 = clientManager.get_user_by_index(index);
	int flight_rules = buf.read_unsigned_byte();
	const std::string assigned_squawk = buf.read_string();
	const std::string departure = buf.read_string();
	const std::string arrival = buf.read_string();
	const std::string alternate = buf.read_string();
	const std::string cruise = buf.read_string();
	const std::string ac_type = buf.read_string();
	const std::string scratch = buf.read_string();
	const std::string route = buf.read_string();
	const std::string remarks = buf.read_string();
	if (user1) {
		User& user_ref = *user1;
		if (user_ref.getType() == PILOT) {
			auto& aircraft = dynamic_cast<Aircraft&>(user_ref);
			FlightPlan& fp = *aircraft.getFlightPlan();
			fp.cycle = cur_cycle;
			fp.squawkCode = assigned_squawk;
			fp.departure = departure;
			fp.arrival = arrival;
			fp.alternate = alternate;
			fp.cruise = cruise;
			fp.acType = ac_type;
			fp.scratchPad = scratch;
			fp.route = route;
			fp.remarks = remarks;
			user_ref.updateOnlyQueues[0] = true;
		}
	}
}

void RequestFlightPlan::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	const int index = buf.read_unsigned_short();
	const int cur_cycle = buf.read_unsigned_short();
	const std::shared_ptr<User> user_fp = clientManager.get_user_by_index(index);
	if (user_fp) {
		if (user_fp->getType() == PILOT) {
			auto& aircraft = dynamic_cast<Aircraft&>(*user_fp);
			if (aircraft.getFlightPlan()->cycle > cur_cycle) {
				send_flight_plan_cycle(*user, aircraft);
			}
		}
	}
}

void PrivateMsgPacket::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	const std::string callsign = buf.read_string();
	const std::shared_ptr<User> user1 = clientManager.get_user_by_callsign(callsign);
	const std::string message = buf.read_string();
	if (user1 && user1 != user) {
		send_private_message(*user1, user->getCallsign(), message.c_str());
	}
}

void PrimeFreqPacket::handle(SOCKET client_socket, const std::shared_ptr<User> user, BasicStream& buf)
{
	int flags = buf.read_unsigned_byte();
	user->frequencies[0] = static_cast<int>(buf.read3Byte());
	user->frequencies[1] = static_cast<int>(buf.read3Byte());
	user->iterateLocalUsers([&](const auto& local) {
		User& users = *local.first;
		if (users.getType() == CONTROLLER) {
			send_prim_freq(users, *user);
		}
		});
}

void TempDataPacket::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	const std::string assembly = buf.read_string();
	std::vector<std::any> objects(assembly.length() + 1);
	for (int i_11_ = assembly.length() - 1; i_11_ >= 0; i_11_--) 
	{
		if (assembly.at(i_11_) == 's')
			objects[i_11_ + 1] = buf.read_string();
		else if (assembly.at(i_11_) == 'l')
			objects[i_11_ + 1] = buf.readQWord();
		else
			objects[i_11_ + 1] = (int)buf.read_unsigned_int();
	}
	objects[0] = (int)buf.read_unsigned_int();
	ClientScript script(assembly);
	script.objects = objects;
	user->registerScript(script);
}

void TempDataRemovePacket::handle(SOCKET client_socket, std::shared_ptr<User> user, BasicStream& buf)
{
	ClientScript& script = *user->getScripts().getScript(buf.read_unsigned_short());
	user->unregisterScript(script);
}

/*if ((Class49.anInt962 ^ 0xffffffff) == -153) {
	RSString class100
		= Class68_Sub13_Sub8.aPacketStream_3560.readString(127);
	Object[] objects
		= new Object[1 + class100.method1590((byte)-119)];
	for (int i_199_ = -1 + class100.method1590((byte)-100);
		i_199_ >= 0; i_199_--) {
		if (class100.method1578(i_199_, 7178) == 115)
			objects[i_199_ - -1]
			= Class68_Sub13_Sub8.aPacketStream_3560
			.readString(i + 26481);
		else
			objects[1 + i_199_] = new Integer(Class68_Sub13_Sub8.aPacketStream_3560.readDWord());
	}
	objects[0] = new Integer(Class68_Sub13_Sub8.aPacketStream_3560.readDWord());
	Class68_Sub29 class68_sub29 = new Class68_Sub29();
	class68_sub29.anObjectArray3237 = objects;
	Class68_Sub13_Sub16.method776(class68_sub29, 16);
	Class49.anInt962 = -1;
	return true;
}*/
