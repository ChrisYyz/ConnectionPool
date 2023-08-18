#include <iostream>

#include "connectionpool.h"
#include "public.h"


ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool;
	return &pool;
}

bool ConnectionPool::loadConfigFile()
{
	FILE* pf = fopen("mysql.ini", "r");
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf))
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);
		std::string str = line;
		int idx = str.find('=', 0);
		if (idx == -1)
		{
			continue;
		}
		int endidx = str.find('\n', idx);
		std::string key = str.substr(0, idx);
		std::string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeout")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;
}

ConnectionPool::ConnectionPool()
{
	if (!loadConfigFile())
	{
		return;
	}
	/* Initialize connection pool */
	for (int i = 0; i < _initSize; i++)
	{
		Connection* p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);
		p->updateAliveTime();
		_connQue.push(p);
		_connCnt++;
	}

	/* Start production thread, for dynamic connection */
	std::thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));
	produce.detach();
	/* Start garbage collection thread */
	std::thread gc(std::bind(&ConnectionPool::deleteConnectionTask, this));
	gc.detach();
}

void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		std::unique_lock<std::mutex> lock(_queMtx);
		while (!_connQue.empty())
		{
			_cv.wait(lock);
		}

		if (_connCnt < _maxSize)
		{
			Connection* p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);
			p->updateAliveTime();
			_connQue.push(p);
			_connCnt++;
		}

		_cv.notify_all();
	}
}

void ConnectionPool::deleteConnectionTask()
{
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::seconds(_maxIdleTime));

		std::unique_lock<std::mutex> lock(_queMtx);
		while (_connCnt > _initSize)
		{
			Connection* p = _connQue.front();
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connQue.pop();
				_connCnt--;
				delete p;
			}
			else
			{
				break;
			}
		}
	}
}

std::shared_ptr<Connection> ConnectionPool::getConnection()
{
	std::unique_lock<std::mutex> lock(_queMtx);
	while (_connQue.empty())
	{
		if (std::cv_status::timeout == _cv.wait_for(lock, std::chrono::milliseconds(_connectionTimeout)))
		{
			if (_connQue.empty())
			{
				LOG("Get connection timeout!");
				return nullptr;
			}
		}
	}

	std::shared_ptr<Connection> sp(_connQue.front(), 
		[&](Connection* pconn) {
			std::unique_lock<std::mutex> lock(_queMtx);
			pconn->updateAliveTime();
			_connQue.push(pconn);
		});
	_connQue.pop();
	_cv.notify_all();
	return sp;
}
