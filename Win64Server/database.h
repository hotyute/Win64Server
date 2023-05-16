#ifndef DATABASE_H
#define DATABASE_H
#include <stdlib.h>
#include <iostream>

#include "User.h"

#include <jdbc/mysql_connection.h>

#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/statement.h>

static sql::Connection* conn; // give access to rest of scope by "static

void connect_to_db();

char* handle_db(char*);
#endif