// user.h
#pragma once
#include <winsock2.h>
#include <string>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "constants.h"

static const int type_intervals = 10;

struct Location {
private:
	double latitude;
	double longitude;
public:
	Location() : latitude(0.0), longitude(0.0) {}

	~Location() {};
	void setLatitude(double value) { latitude = value; }
	double getLatitude() const { return latitude; }
	void setLongitude(double value) { longitude = value; }
	double getLongitude() const { return longitude; }
};

struct Identity {
	std::string callsign;
	std::string login_name;
	std::string password;
	std::string username;
	~Identity() {};
};

class User {
public:
	User(SOCKET clientSocket, int id);
	bool updateOnlyQueues[4];
	int frequencies[2];
	std::unordered_map<std::shared_ptr<User>, std::vector<void*>> local_users;
	std::shared_mutex local_mutex;
	std::chrono::steady_clock::time_point last_pong;
	std::mutex send_data_mutex;
	virtual ~User() = default;
	virtual void update() = 0;
	int getIndex() const { return index; }
	void setIndex(int value) { index = value; }
	int getId() const { return id; }
	SOCKET getClientSocket() const { return socket; }
	Identity& getIdentity() { return identity; }
	Location& getLocation() { return location; }
	std::string getCallsign() { return identity.callsign; }
	const int getVisibilityRange() const { return visibilityRange; }
	void setVisibilityRange(int val) { visibilityRange = val; }
	const bool isUpdateRequired() const { return updateRequired; }
	void setUpdateRequired(bool updateRequired) { updateRequired = updateRequired; }
	const bool isTriggerUpdate() const { return triggerUpdate; }
	void setTriggerUpdate(bool triggerUpdate) { triggerUpdate = triggerUpdate; }
	long long getRequestedInterval(int type) { return requestTime[type]; }
	void setRequestedInterval(int type, long long request) { requestTime[type] = request; }
	void setAllRequestedIntervals(long long request) { for (int i = 0; i < type_intervals; i++) { requestTime[i] = request; } }
	const long long getUpdateInterval() const { return updateInterval; }
	void setUpdateInterval(long long val) { updateInterval = val; }
	const AV_CLIENT getType() const { return type; }
	void setType(AV_CLIENT clientType) { type = clientType; }
	const long long getLastPositionUpdate() const { return last_position_update; }
	void setLastPositionUpdate(long long value) { last_position_update = value; }
	bool isValid() const { return valid; }
	void setValid(bool value) { valid = value; }
	template <typename Function>
	void iterateLocalUsers(Function func) {
		std::shared_lock<std::shared_mutex> lock(local_mutex);

		for (const auto& local_user_pair : local_users) {
			func(local_user_pair);
		}
	}


protected:
	SOCKET socket;
	Identity identity;
	Location location;
	AV_CLIENT type;
	int index = -1;
	int id = 0;
	int visibilityRange = 300;
	long long requestTime[type_intervals];
	long long updateInterval;
	long long last_position_update;
	bool updateRequired, triggerUpdate;
	bool valid = true;
};
