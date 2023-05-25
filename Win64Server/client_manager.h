#pragma once

#include <winsock2.h>
#include <queue>
#include <memory>
#include <mutex>
#include <shared_mutex>

#include <unordered_map>

#include "aircraft.h"
#include "controller.h"
#include "constants.h"

class ClientManager {
public:
	explicit ClientManager(const int max_clients) {
		for (int i = 0; i < max_clients; ++i) {
			available_indices_.push(i);
		}
	}

	int get_available_index() {
		std::unique_lock<std::shared_mutex> lock(queue_mutex_);
		if (available_indices_.empty()) {
			return -1;
		}

		const int index = available_indices_.front();
		available_indices_.pop();
		return index;
	}

	std::shared_ptr<User> initial_user(SOCKET client_socket, const int user_type, int id) const
	{
		std::shared_ptr<User> user;
		if (user_type == PILOT) {
			user = std::make_shared<Aircraft>(client_socket, id);
		}
		else if (user_type == CONTROLLER) {
			user = std::make_shared<Controller>(client_socket, id);
		}
		else {
			return nullptr;
		}
		return user;
	}

	int provide_index() {
		const int index = get_available_index();
		if (index == -1) {
			return index;
		}

		return index;
	}

	void provide_placement(const SOCKET client_socket, const std::shared_ptr<User>& user) {
		std::unique_lock<std::shared_mutex> lock_client(client_mutex_);
		std::unique_lock<std::shared_mutex> lock_id(id_mutex_);
		std::unique_lock<std::shared_mutex> lock_index(index_mutex_);
		std::unique_lock<std::shared_mutex> lock_callsign(callsign_mutex_);

		clients_[client_socket] = user;
		users_by_id_[user->getId()] = user;
		users_by_index_[user->getIndex()] = user;
		users_by_callsign[user->getIdentity().callsign] = user;
	}


	void remove_user(const SOCKET client_socket) {
		std::unique_lock<std::shared_mutex> lock_client(client_mutex_);

		const auto it = clients_.find(client_socket);
		if (it != clients_.end()) {
			std::unique_lock<std::shared_mutex> lock_id(id_mutex_);
			std::unique_lock<std::shared_mutex> lock_index(index_mutex_);
			std::unique_lock<std::shared_mutex> lock_callsign(callsign_mutex_);
			std::unique_lock<std::shared_mutex> lock_queue(queue_mutex_);

			available_indices_.push(it->second->getIndex());
			users_by_index_.erase(it->second->getIndex());
			users_by_id_.erase(it->second->getId());
			users_by_callsign.erase(it->second->getIdentity().callsign);
			clients_.erase(it);
		}
	}

	std::shared_ptr<User> get_user_by_id(const int id) const {
		std::shared_lock<std::shared_mutex> lock(id_mutex_);
		const auto it = users_by_id_.find(id);
		if (it != users_by_id_.end()) {
			return it->second;
		}
		return nullptr;
	}

	template <typename Func>
	void iterate_users(Func func) const {
		std::shared_lock<std::shared_mutex> lock(client_mutex_);
		for (const auto& pair : clients_) {
			func(pair.second);
		}
	}


	std::shared_ptr<User> get_user_by_index(const int index) const {
		std::shared_lock<std::shared_mutex> lock(index_mutex_);
		const auto it = users_by_index_.find(index);
		if (it != users_by_index_.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<User> get_user_by_socket(const SOCKET client_socket) const {
		std::shared_lock<std::shared_mutex> lock(client_mutex_);
		const auto it = clients_.find(client_socket);
		if (it != clients_.end()) {
			return it->second;
		}
		return nullptr;
	}

	std::shared_ptr<User> get_user_by_callsign(const std::string& callsign) const {
		std::shared_lock<std::shared_mutex> lock(callsign_mutex_);
		const auto it = users_by_callsign.find(callsign);
		if (it != users_by_callsign.end()) {
			return it->second;
		}
		return nullptr;
	}

	mutable std::shared_mutex client_mutex_;
	std::unordered_map<SOCKET, std::shared_ptr<User>> clients_;

private:
	std::unordered_map<int, std::shared_ptr<User>> users_by_id_;
	std::unordered_map<int, std::shared_ptr<User>> users_by_index_;
	std::unordered_map<std::string, std::shared_ptr<User>> users_by_callsign;
	std::queue<int> available_indices_;
	mutable std::shared_mutex id_mutex_;
	mutable std::shared_mutex index_mutex_;
	mutable std::shared_mutex callsign_mutex_;
	std::shared_mutex queue_mutex_;
};

// Instantiate the ClientManager with a maximum number of clients
constexpr int MAX_CLIENTS = 250;
extern ClientManager clientManager;

void adjust_time(const std::shared_ptr<User>& user);
