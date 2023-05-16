#include "database.h"

void connect_to_db()
{
	try 
	{
		static sql::Driver* driver;

		driver = get_driver_instance();
		conn = driver->connect("tcp://127.0.0.1:3306", "root", "");
		conn->setSchema("fsd2");
	} 
	catch (sql::SQLException& e) 
	{
		//std::cout << "# ERR: SQLException in " << __FILE__;
		//std::cout << "(" << __FUNCTION__ << ") on line " »
		//	<< __LINE__ << std::endl;
		std::cout << "# ERR: " << e.what();
		std::cout << " (MySQL error code: " << e.getErrorCode();
		std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
}

char* handle_db(char* username)
{
	char* password = nullptr;
	try
	{
		sql::Statement* stmt;
		sql::ResultSet* res;
		stmt = conn->createStatement();
		res = stmt->executeQuery("SELECT * FROM  users WHERE username = " + std::string(username));
		while (res->next()) {
			res->getString("password").c_str();
		}
	}
	catch (sql::SQLException& e) 
	{
		//std::cout << "# ERR: SQLException in " << __FILE__;
		//std::cout << "(" << __FUNCTION__ << ") on line " »
		//	<< __LINE__ << std::endl;
		std::cout << "# ERR: " << e.what();
		std::cout << " (MySQL error code: " << e.getErrorCode();
		std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
	return password;
}
