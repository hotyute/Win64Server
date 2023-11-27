#include "user.h"

#include <functional>

#include "outgoing_packets.h"

User::User(SOCKET clientSocket, int id) : socket(clientSocket), type(), id(id)
{
	User::last_pong = std::chrono::high_resolution_clock::now();

	std::fill(std::begin(updateOnlyQueues), std::end(updateOnlyQueues), false);

	std::fill(std::begin(frequencies), std::end(frequencies), 99998);
}

void User::processScript(ClientScript script)
{
	User& user = *this;
	getScripts().addScript(std::make_shared<ClientScript>(script));
	send_script(user, user, script);
	iterateLocalUsers([&](const auto& local) {
		User& other = *local.first;
		send_script(other, user, script);
	});
}



