#include "client_updater.h"

#include "tools.h"
#include "outgoing_packets.h"

#include <iostream>
#include <winsock2.h>
#include <vector>
#include <cstdint>
#include <mutex>

#include "Stream.h"

extern std::shared_mutex clientSocketsMutex;
extern std::vector<SOCKET> clientSockets;

UpdateTask::UpdateTask(ThreadPool& threadPool, std::chrono::milliseconds duration)
	: TimedTask(threadPool, duration) {}

UpdateTask::~UpdateTask() = default;

void UpdateTask::execute() {
	get_thread_pool().enqueue([&]
		{
			globalUpdate();
		});
}

void globalUpdate()
{

	//update local users first (add and delete users around the main user)
	{
		clientManager.iterate_users([&](const std::shared_ptr<User>& user) {
			if (user) {
				updateLocalUsers(user);

				//flags
				if (user->isTriggerUpdate()) {
					user->setUpdateRequired(true);
					user->setTriggerUpdate(false);
				}
			}
			});
	}

	//update users now (which basically means updating the user's local users about main user)
	clientManager.iterate_users([&](const std::shared_ptr<User>& user) {
		if (user) {
			update(user);
			//check_timeout(user);
		}
		});

	//updates that can only happen AFTER an update 
	clientManager.iterate_users([&](const std::shared_ptr<User>& user) {
		if (user) {
			updateOnlyQueues(*user);
		}
		});

	//finally reset players that required update after global update
	clientManager.iterate_users([&](const std::shared_ptr<User>& user) {
		if (user) {
			user->setUpdateRequired(false);
		}
		});
}

void updateLocalUsers(const std::shared_ptr<User>&user) {
	if (user) {
		const double latitude = user->getLocation().getLatitude();
		const double longitude = user->getLocation().getLongitude();
		for (const auto& pair : clientManager.clients_) {
			std::shared_ptr<User> user2 = pair.second;
			if (user2 && user != user2) {
				const double latitude2 = user2->getLocation().getLatitude();
				const double longitude2 = user2->getLocation().getLongitude();
				const double distance = dist(latitude, longitude, latitude2, longitude2);
				if (!contains_user(user, user2) && distance <= user->getVisibilityRange()) {
					const long long reqInterval = user->getRequestedInterval(user2->getType());
					if (user2->getUpdateInterval() > reqInterval) {
						user2->setUpdateInterval(reqInterval);
						send_time_change(*user2, reqInterval);
					}
					create_client(*user, *user2);
					create_flight_plan(*user, *user2);
					{
						std::unique_lock<std::shared_mutex> lock(user->local_mutex);
						user->local_users.emplace(user2, std::vector<void*>(NUM_ATT));
					}
				}
				else if (contains_user(user, user2) && distance > user->getVisibilityRange()) {
					delete_client(*user, *user2);
					{
						std::unique_lock<std::shared_mutex> lock(user->local_mutex);
						user->local_users.erase(user2);
					}

					adjust_time(user2);
				}
			}
		}
	}
}

void update(const std::shared_ptr<User>&user) {
	user->iterateLocalUsers([&](const auto& local_user) {
		//Update Time
		const std::shared_ptr<User> other = local_user.first;
		if (!other || !user)
			return;

		const auto userReqInterval = user->getRequestedInterval(other->getType());
		const auto time = timeUpdate(other, user);
		if (time != nullptr) {
			//something?
		}
		std::vector<void*> att = local_user.second;
		const auto otherUpdateStamp = other->getLastPositionUpdate();
		void* last = att[LAST_POS_UPDATE];//TODO Bug over here when someone else disconnects, invalid parameter passed, perhaps synch?
		if (other->getUpdateInterval() > userReqInterval) {
			std::cout << "Interval Requested is Lower than current Update Interval." << std::endl;
		}
		else
		{
			if (user->isUpdateRequired()) {
				//don't need to do an interval check, since the user has already been marked as needing an update
				//user gets updated of surrounding player's positions
				send_pos_update(user, other);
				att[LAST_POS_UPDATE] = reinterpret_cast<void*>(otherUpdateStamp);
				user->local_users[other] = att;
			}
			else {
				if (last) {
					const auto user_other_update_stamp = reinterpret_cast<long long>(last);
					const auto now = boost::posix_time::microsec_clock::local_time().time_of_day().total_milliseconds();
					const auto interval = (now - user_other_update_stamp);
					//if the timestamp changed from last update it means the other player updated
					//and if last interval meets user's requested time
					if (user_other_update_stamp != otherUpdateStamp && interval >= userReqInterval) {
						const long long minTime = userReqInterval - other->getUpdateInterval();
						const long long maxTime = userReqInterval + other->getUpdateInterval();
						if (interval >= minTime && interval <= maxTime) {
							//it is within range for update
							send_pos_update(user, other);
							att[LAST_POS_UPDATE] = reinterpret_cast<void*>(otherUpdateStamp);
							user->local_users[other] = att;
						}
						else if (interval > maxTime) {
							//it has overlapped the time range for update (long overdue for update)
							//it should never overlap, however just in-case we need to inform all other players of overlap
							send_pos_update(other, user);
							att[LAST_POS_UPDATE] = reinterpret_cast<void*>(otherUpdateStamp);
							user->local_users[other] = att;
						}
						else {
							//It has not yet within range!!
						}
					}
				}
				else {
					//we have not updated yet, so update now
					//to prevent CPU intense calculations in interval, we send an update to other players on initial update
					send_pos_update(other, user);
					att[LAST_POS_UPDATE] = reinterpret_cast<void*>(otherUpdateStamp);
					user->local_users[other] = att;
				}
			}
		}
		});
}

void send_pos_update(const std::shared_ptr<User>&updated_user, const std::shared_ptr<User>&about_user) {
	if (about_user->getType() == PILOT_CLIENT)
	{
		const auto aircraft = dynamic_cast<Aircraft*>(about_user.get());
		if (aircraft) {
			send_pilot_update(*updated_user, *aircraft);
		}
	}
	else if (about_user->getType() == CONTROLLER_CLIENT)
	{
		const auto controller = dynamic_cast<Controller*>(about_user.get());
		if (controller) {
			send_controller_update(*updated_user, *controller);
		}
	}
}

void updateOnlyQueues(User & user) {
	//TODO move this somewhere else
	if (user.getType() == AV_CLIENT::PILOT)
	{
		if (user.updateOnlyQueues[0])
		{
			const FlightPlan& fp = *dynamic_cast<Aircraft&>(user).getFlightPlan();
			user.iterateLocalUsers([&](const auto& local) {
				User& other = *local.first;
				if (other.getType() == AV_CLIENT::CONTROLLER && fp.cycle) {
					send_flight_plan_cycle(other, user);
				}
			});
			user.updateOnlyQueues[0] = false;
		}
	}
}

bool contains_user(const std::shared_ptr<User>&user, const std::shared_ptr<User>&user2) {
	std::shared_lock<std::shared_mutex> lock(user->local_mutex);
	return (user->local_users.find(user2) != user->local_users.end());
}

std::shared_ptr<long long> timeUpdate(const std::shared_ptr<User>&user, const std::shared_ptr<User>&other) {
	const long long otherReqInterval = other->getRequestedInterval(user->getType());
	if (user->getUpdateInterval() > otherReqInterval) {
		user->setUpdateInterval(otherReqInterval);
		send_time_change(*user, otherReqInterval);
		return std::make_shared<long long>(otherReqInterval);
	}
	// Returned the current user's update interval if it's less than or equal to the other user's
	return std::make_shared<long long>(user->getUpdateInterval());
}

void check_timeout(const std::shared_ptr<User>&user)
{
	const auto now = std::chrono::high_resolution_clock::now();

	const std::chrono::duration<double, std::milli> elapsed = now - user->last_pong;

	if (elapsed.count() >= 11000)
	{
		printf("[--User: %s has timed out--]\n", user->getIdentity().username.c_str());
		//handle_disconnect(user);
	}
}
