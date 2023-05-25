#include <boost/algorithm/string.hpp>

#include "handshake.h"
#include "user.h"
#include "client_manager.h"
#include "aircraft.h"
#include "controller.h"
#include "packet_handler.h"
#include "tools.h"

HandshakePacket examplePacket(45, -2);

void HandshakePacket::handle(SOCKET clientSocket, std::shared_ptr<User> user, BasicStream& buf)
{
	int proto_version = static_cast<int>(buf.read_unsigned_int());
	const std::string call_sign = buf.read_string();
	const std::string loginName = buf.read_string();
	const std::string username = buf.read_string();
	const std::string password = buf.read_string();
	long long request_time = buf.readQWord();
	double latitude = longToRawBits(buf.readQWord());
	double longitude = longToRawBits(buf.readQWord());
	int visibility = buf.read_unsigned_short();
	int type = buf.read_unsigned_byte();
	int frequencies[2];
	std::string acfTitle, transponder;
	frequencies[0] = buf.read3Byte();
	frequencies[1] = buf.read3Byte();
	int controller_rating = 0, controller_position = 0, controller_prim = 99998;
	int pilotRating = 0;
	int mode = 0;
	double pitch = 0.0, roll = 0.0, heading = 0.0;
	if (type == CONTROLLER)
	{
		controller_rating = buf.read_unsigned_byte();
		controller_position = buf.read_unsigned_byte();
	}
	else if (type == PILOT)
	{
		pilotRating = buf.read_unsigned_byte();
		acfTitle = buf.read_string();
		transponder = buf.read_string();
		mode = buf.read_unsigned_byte();
		long long hash = buf.readQWord();
		pitch = static_cast<int>(hash >> 22) / 1024.0 * -360.0;
		roll = static_cast<int>(hash >> 12 & 0x3ff) / 1024.0 * -360.0;
		heading = static_cast<int>(hash >> 2 & 0x3ff) / 1024.0 * 360.0;
	}

	const char* cmp_password = password.c_str();//handle_db(username); //&password[0];

						//create USER
	std::shared_ptr<User> new_user = nullptr;

	int response = 1;

	if (boost::iequals(cmp_password, password))
	{
		int id = std::stoi(username);
		// Create a new User object (Aircraft or Controller) and add it to the container
		new_user = clientManager.initial_user(clientSocket, type, id);
		new_user->setType(static_cast<AV_CLIENT>(type));

		new_user->getIdentity().callsign = std::string(call_sign);
		new_user->getIdentity().login_name = std::string(loginName);
		new_user->getIdentity().username = std::string(username);
		new_user->getIdentity().password = std::string(password);

		if (type == PILOT) {
			std::shared_ptr<Aircraft> aircraft = std::static_pointer_cast<Aircraft>(new_user);
			aircraft->pilot_rating = pilotRating;
			aircraft->setAcfTitle(acfTitle);
			aircraft->setTransponder(transponder);
			aircraft->setMode(mode);
			aircraft->getState().setPitch(pitch);
			aircraft->getState().setRoll(roll);
			aircraft->getState().setHeading(heading);
			aircraft->createFlightPlan();
		}
		else if (type == CONTROLLER) {
			std::shared_ptr<Controller> controller = std::static_pointer_cast<Controller>(new_user);
			controller->controller_rating = controller_rating;
			controller->controller_position = controller_position;
			controller->frequencies[0] = controller_prim;
		}
		else if (new_user == nullptr) {
			// Close the connection or handle the error
		}
		std::copy(std::begin(frequencies), std::end(frequencies), std::begin(new_user->frequencies));
		new_user->getLocation().setLatitude(latitude);
		new_user->getLocation().setLongitude(longitude);
		new_user->setAllRequestedIntervals(request_time);
		new_user->setVisibilityRange(visibility);
		new_user->setUpdateInterval(DEFAULT);
	}
	else {
		response = 3;
	}

	if (proto_version != PROTO_VERSION) {
		response = 2;
	}

	if (new_user)
	{
		new_user->setIndex(clientManager.provide_index());
		if (response == 1)
		{
			
			BasicStream out = BasicStream(11);
			out.write_byte(response);
			out.write_short(new_user->getIndex());//index
			out.write_qword(new_user->getUpdateInterval());
			write(*new_user, out);

			send_server_message(*new_user, "Welcome to the FSD 2.0 Test Server!");
			send_server_message(*new_user, "Please ensure that you adhere to developer's rules and direction.");

			printf("[--User: %s <%s> connected--]\n", new_user->getCallsign().c_str(), new_user->getIdentity().username.c_str());
			clientManager.provide_placement(clientSocket, new_user);
		}
		else
		{
			BasicStream out = BasicStream(1);
			out.write_byte(response);
			write(*new_user, out);
			removeClientSocket(clientSocket, true);
			printf("[--User: is response: %d--]\n", response);
		}
	}
}
