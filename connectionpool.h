#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <memory>
#include <functional>

#include "connection.h"
// **************************** connection pool functions... ************************

class ConnectionPool
{
public:
	static ConnectionPool* getConnectionPool();
	std::shared_ptr<Connection> getConnection();
private:
	ConnectionPool();
public:
	bool loadConfigFile();

	void produceConnectionTask();
	void deleteConnectionTask();

	std::string _ip;
	unsigned short _port;
	std::string _username; 
	std::string _password; 
	std::string _dbname;
	int _initSize; 
	int _maxSize; 
	int _maxIdleTime; 
	int _connectionTimeout;

	std::atomic_int _connCnt;
	std::queue<Connection*> _connQue;
	std::mutex _queMtx;
	std::condition_variable _cv;
};