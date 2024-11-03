#include "conn_pool.h"

connection_pool::connection_pool(std::string conn_str, size_t conn_cnt)
{
	this->conn_str = conn_str;
	this->conn_cnt = conn_cnt;
	for(size_t i = 0; i < conn_cnt; ++i)
		pool.emplace(std::make_shared<pqxx::connection>(pqxx::connection{conn_str}));
}

std::shared_ptr<pqxx::connection> connection_pool::hold()
{
	std::unique_lock<std::mutex> lock{q_mutex};
	while(pool.empty())
		q_cond.wait(lock);
	std::shared_ptr<pqxx::connection> conn = pool.front();
	pool.pop();
	return conn;
}
void connection_pool::release(std::shared_ptr<pqxx::connection> conn)
{
	std::unique_lock<std::mutex> lock{q_mutex};
	pool.push(conn);
	lock.unlock();
	q_cond.notify_one();
}
