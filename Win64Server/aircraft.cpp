#include "aircraft.h"
#include <iostream>

Aircraft::Aircraft(SOCKET clientSocket, int id)
    : User(clientSocket, id)
{
}

void Aircraft::update() {
    // Update position, speed, etc.
    std::cout << "Aircraft updated" << std::endl;
}

FlightPlan::FlightPlan()
{
	FlightPlan::squawkCode = "0000";
	FlightPlan::departure = "";
	FlightPlan::arrival = "";
	FlightPlan::alternate = "";
	FlightPlan::squawkCode = "";
	FlightPlan::acType = "";
	FlightPlan::scratchPad = "";
	FlightPlan::cruise = "";
	FlightPlan::route = "";
	FlightPlan::remarks = "";
}
