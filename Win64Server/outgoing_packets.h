#pragma once

#include <winsock2.h>
#include <vector>
#include <memory>

#include "packet.h"
#include "user.h"
#include "aircraft.h"
#include "controller.h"
#include "Stream.h"

void send_ping(User& user);

void send_pilot_update(User& user, Aircraft& other);

void send_message(User& user, const User& from, int frequency, const char* message, bool asel);

void send_client_mode_change(User& user, const Aircraft& other);

void send_flight_plan_cycle(User& user, User& from);

void send_controller_update(User& user, Controller& other);

void send_client_transponder(User& user, const Aircraft& other);

void send_prim_freq(User& user, const User& other);

void send_script(User& user, const User& other, ClientScript& script);

void send_server_message(User& user, const char* message);

void send_wx(User& user, const char* weather);

void create_client(User& user, User& client);

void create_flight_plan(User& user, User& client);

void send_time_change(User& user, long long time);

void send_private_message(User& user, const std::string& from, const char* message);

void delete_client(User& user, const User& client);

void write(User& user, BasicStream& stream);

void send_data(SOCKET clientSocket, const std::vector<char>& buffer);
