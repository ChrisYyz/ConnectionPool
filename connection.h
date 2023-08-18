#pragma once

// *********************** MySQL CRUD functions... ****************************************
#include <mysql.h>
#include <string>
#include <ctime>

class Connection
{
public:
	Connection();
	~Connection();

	bool connect(std::string ip, 
				 unsigned short port, 
				 std::string user,
				 std::string password,
				 std::string dbname);
	bool update(std::string sql);

	MYSQL_RES* query(std::string sql);
	void updateAliveTime() { _aliveTime = clock(); }
	clock_t getAliveTime() const { return clock() - _aliveTime; }
private:
	MYSQL* _conn;
	clock_t _aliveTime;
};