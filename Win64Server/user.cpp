#include "user.h"

#include <functional>

#include "outgoing_packets.h"

User::User(SOCKET clientSocket, int id) : socket(clientSocket), type(), id(id)
{
	User::last_pong = std::chrono::high_resolution_clock::now();

	std::fill(std::begin(updateOnlyQueues), std::end(updateOnlyQueues), false);

	std::fill(std::begin(frequencies), std::end(frequencies), 99998);
}

void User::processScript(const ClientScript& pscript)
{
	auto sscript = std::make_shared<ClientScript>(pscript);
	getScripts().addScript(sscript);

	send_script(*this, *this, *sscript);
	iterateLocalUsers([&](const auto& local) {
		send_script(*local.first, *this, *sscript);
	});
}



