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
	int proto_version = buf.read_unsigned_int();
	const std::string callSign = buf.read_string();
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
	if (type == AV_CLIENT::CONTROLLER)
	{
		controller_rating = buf.read_unsigned_byte();
		controller_position = buf.read_unsigned_byte();
	}
	else if (type == AV_CLIENT::PILOT)
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

	//printf("[--User: %s <%s> connected--]\n", callSign, username);

	const char* cmpPassword = password.c_str();//handle_db(username); //&password[0];

						//create USER
	std::shared_ptr<User> newUser;

	int response = 1;

	if (cmpPassword != nullptr)
	{
		if (boost::iequals(cmpPassword, password))
		{
			int id = 0;
			// Create a new User object (Aircraft or Controller) and add it to the container
			newUser = clientManager.initialUser(clientSocket, type, id);
			newUser->setType(static_cast<AV_CLIENT>(type));

			newUser->getIdentity().callsign = std::string(callSign);
			newUser->getIdentity().login_name = std::string(loginName);
			newUser->getIdentity().username = std::string(username);
			newUser->getIdentity().password = std::string(password);

			clientManager.usersByCallsign[newUser->getIdentity().callsign] = newUser;

			if (type == AV_CLIENT::PILOT) {
				std::shared_ptr<Aircraft> aircraft = std::static_pointer_cast<Aircraft>(newUser);
				aircraft->pilot_rating = pilotRating;
				aircraft->setAcfTitle(acfTitle);
				aircraft->setTransponder(transponder);
				aircraft->setMode(mode);
				aircraft->getState().setPitch(pitch);
				aircraft->getState().setRoll(roll);
				aircraft->getState().setHeading(heading);
				aircraft->createFlightPlan();
			}
			else if (type == AV_CLIENT::CONTROLLER) {
				std::shared_ptr<Controller> controller = std::static_pointer_cast<Controller>(newUser);
				controller->controller_rating = controller_rating;
				controller->controller_position = controller_position;
				controller->frequencies[0] = controller_prim;
			}
			else if (newUser == nullptr) {
				// Close the connection or handle the error
			}
			std::copy(std::begin(frequencies), std::end(frequencies), std::begin(newUser->frequencies));
			newUser->getLocation().setLatitude(latitude);
			newUser->getLocation().setLongitude(longitude);
			newUser->setAllRequestedIntervals(request_time);
			newUser->setVisibilityRange(visibility);
			newUser->setUpdateInterval(UPDATE_TIMES::DEFAULT);
		}
		else {
			response = 3;
		}
	}

	if (proto_version != VERSIONS::PROTO_VERSION) {
		response = 2;
	}
	if (newUser)
	{
		if (response == 1)
		{
			newUser = clientManager.createUser(clientSocket, newUser);
			Stream out = Stream(11);
			out.writeByte(response);
			out.writeWord(newUser->getIndex());//index
			out.writeQWord(newUser->getUpdateInterval());
			write(*newUser, out);

			send_server_message(*newUser, "Welcome to the FSD 2.0 Test Server!");
			send_server_message(*newUser, "Please ensure that you adhere to developer's rules and direction.");

			printf("[--User: %s <%s> connected--]\n", newUser->getCallsign().c_str(), newUser->getIdentity().username.c_str());
		}
		else
		{
			Stream out = Stream(1);
			out.writeByte(response);
			write(*newUser, out);
			removeClientSocket(clientSocket, true);
		}
	}
}
