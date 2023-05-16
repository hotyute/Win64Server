// aircraft.h
#pragma once
#include "user.h"

class FlightPlan {
public:
	FlightPlan();
	std::string departure, arrival, alternate, squawkCode;
	std::string acType, scratchPad, cruise, route, remarks;
	int flightRules;
	int cycle;
};

struct State {
private:
	double heading, roll, pitch;
	int indicatedSpeed, groundSpeed;
	double altitude;
public:
	~State() {};
	double getRoll() const {
		return roll;
	}
	void setRoll(double roll) {
		State::roll = roll;
	}
	double getPitch() const {
		return pitch;
	}
	void setPitch(double pitch) {
		State::pitch = pitch;
	}
	int getIndicatedSpeed() const {
		return indicatedSpeed;
	}
	void setIndicatedSpeed(int indicatedSpeed) {
		State::indicatedSpeed = indicatedSpeed;
	}
	double getAltitude() const {
		return altitude;
	}
	void setAltitude(double altitude) {
		State::altitude = altitude;
	}
	double getHeading() const {
		return heading;
	}
	void setHeading(double heading) {
		State::heading = heading;
	}
	int getGroundSpeed() const {
		return groundSpeed;
	}
	void setGroundSpeed(int groundSpeed) {
		State::groundSpeed = groundSpeed;
	}
};

class Aircraft : public User {
public:
    Aircraft(SOCKET clientSocket, int id);

    void update() override;

	int pilot_rating = 0;
	bool heavy = false;

	void setTransponder(std::string transponder) { Aircraft::transponder = transponder; }
	std::string getTransponder() const { return Aircraft::transponder; }
	void setAcfTitle(std::string title) { acfTitle = title; }
	std::string getAcfTitle() const { return acfTitle; }
	State& getState() { return state; }
	void setMode(int mode) { Aircraft::mode = mode; }
	const int getMode() const { return Aircraft::mode; }
	FlightPlan* getFlightPlan() { return flight_plan; }
	void createFlightPlan() { flight_plan = new FlightPlan(); }

private:
    std::string transponder;
	std::string acfTitle;
	State state;
	int mode;
	FlightPlan* flight_plan;
};
