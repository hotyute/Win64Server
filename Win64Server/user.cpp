#include "user.h"

#include <functional>

User::User(SOCKET clientSocket, int id) : socket(clientSocket), type(), id(id)
{
	User::last_pong = std::chrono::high_resolution_clock::now();

	std::fill(std::begin(updateOnlyQueues), std::end(updateOnlyQueues), false);

	std::fill(std::begin(frequencies), std::end(frequencies), 99998);
}



