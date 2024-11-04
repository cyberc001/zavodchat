#include "conn_pool.h"

db_connection::db_connection(std::shared_ptr<pqxx::connection> conn, db_connection_pool& pool): conn{conn}, pool{pool}
{}
db_connection::~db_connection()
{
	pool.release(conn);
}
pqxx::connection& db_connection::operator*()
{
	return *conn;
}

db_connection_pool::db_connection_pool(std::string conn_str, size_t conn_cnt)
{
	this->conn_str = conn_str;
	this->conn_cnt = conn_cnt;
	for(size_t i = 0; i < conn_cnt; ++i)
		pool.emplace(std::make_shared<pqxx::connection>(pqxx::connection{conn_str}));
}

db_connection db_connection_pool::hold()
{
	std::unique_lock<std::mutex> lock{q_mutex};
	while(pool.empty())
		q_cond.wait(lock);
	std::shared_ptr<pqxx::connection> conn = pool.front();
	pool.pop();
	return db_connection{conn, *this};
}
void db_connection_pool::release(std::shared_ptr<pqxx::connection> conn)
{
	std::unique_lock<std::mutex> lock{q_mutex};
	pool.push(conn);
	lock.unlock();
	q_cond.notify_one();
}
