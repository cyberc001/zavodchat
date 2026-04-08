#include "conn_pool.h"

inner_db_connection::inner_db_connection(std::string conn_str):
	pqc(conn_str)
{}

db_connection::db_connection(std::shared_ptr<inner_db_connection> conn, db_connection_pool& pool):
	conn{conn}, pool{pool}
{}
db_connection::~db_connection()
{
	pool.release(conn);
}
pqxx::connection& db_connection::operator*()
{
	return conn->pqc;
}

bool db_connection::prepare(std::string name, std::string query)
{
	if(conn->prepared_statements.count(name))
		return false;
	conn->prepared_statements.insert(name);
	conn->pqc.prepare(name, query);
	return true;
}

db_connection_pool::db_connection_pool(std::string conn_str, size_t conn_cnt)
{
	this->conn_str = conn_str;
	this->conn_cnt = conn_cnt;
	for(size_t i = 0; i < conn_cnt; ++i)
		pool.emplace(std::make_shared<inner_db_connection>(conn_str));
}

db_connection db_connection_pool::hold()
{
	std::unique_lock<std::mutex> lock{q_mutex};
	while(pool.empty())
		q_cond.wait(lock);
	std::shared_ptr<inner_db_connection> conn = pool.front();
	pool.pop();
	return db_connection(conn, *this);
}
void db_connection_pool::release(std::shared_ptr<inner_db_connection> conn)
{
	std::unique_lock<std::mutex> lock{q_mutex};
	pool.push(conn);
	lock.unlock();
	q_cond.notify_one();
}
