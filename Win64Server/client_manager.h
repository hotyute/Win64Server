#pragma once

#include <winsock2.h>
#include <queue>
#include <memory>
#include <mutex>
#include <iostream>
#include <shared_mutex>

#include <unordered_map>

#include "aircraft.h"
#include "controller.h"
#include "constants.h"

class ClientManager {
public:
	ClientManager(int maxClients) {
		for (int i = 0; i < maxClients; ++i) {
			availableIndices.push(i);
		}
	}

	int getAvailableIndex() {
		if (availableIndices.empty()) {
			return -1;
		}

		int index = availableIndices.front();
		availableIndices.pop();
		return index;
	}

	std::shared_ptr<User> initialUser(SOCKET clientSocket, int userType, int id) {
		std::shared_ptr<User> user;
		if (userType == AV_CLIENT::PILOT) {
			user = std::make_shared<Aircraft>(clientSocket, id);
		}
		else if (userType == AV_CLIENT::CONTROLLER) {
			user = std::make_shared<Controller>(clientSocket, id);
		}
		else {
			return nullptr;
		}
		return user;
	}

	std::shared_ptr<User> createUser(SOCKET clientSocket, std::shared_ptr<User> user) {
		std::unique_lock<std::shared_mutex> lock(mutex);

		int index = getAvailableIndex();
		if (index == -1) {
			return nullptr;
		}

		user->setIndex(index);

		clients[clientSocket] = user;
		usersById[user->getId()] = user;
		usersByIndex[index] = user;
		return user;
	}


	void removeUser(SOCKET clientSocket) {
		std::unique_lock<std::shared_mutex> lock(mutex);
		auto it = clients.find(clientSocket);
		if (it != clients.end()) {
			availableIndices.push(it->second->getIndex());
			usersById.erase(it->second->getId());
			usersByIndex.erase(it->second->getIndex());
			usersByCallsign.erase(it->second->getIdentity().callsign);
			clients.erase(it);
		}
	}

	std::shared_ptr<User> getUserById(int id) const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		auto it = usersById.find(id);
		if (it != usersById.end()) {
			return it->second;
		}
		return nullptr;
	}

	template <typename Func>
	void iterateUsers(Func func) {
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (const auto& pair : clients) {
			func(pair.second);
		}
	}


	std::shared_ptr<User> getUserByIndex(int index) const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		auto it = usersByIndex.find(index);
		if (it != usersByIndex.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<User> getUserBySocket(SOCKET clientSocket) const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		auto it = clients.find(clientSocket);
		if (it != clients.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<User> getUserByCallsign(const std::string& callsign) const {
		std::shared_lock<std::shared_mutex> lock(mutex);
		auto it = usersByCallsign.find(callsign);
		if (it != usersByCallsign.end()) {
			return it->second;
		}
		return nullptr;
	}

public:
	std::unordered_map<std::string, std::shared_ptr<User>> usersByCallsign;

private:
	std::unordered_map<SOCKET, std::shared_ptr<User>> clients;
	std::unordered_map<int, std::shared_ptr<User>> usersById;
	std::unordered_map<int, std::shared_ptr<User>> usersByIndex;
	std::queue<int> availableIndices;
	mutable std::shared_mutex mutex;
};

// Instantiate the ClientManager with a maximum number of clients
const int MAX_CLIENTS = 250;
extern ClientManager clientManager;

void adjust_time(const std::shared_ptr<User>& user);
