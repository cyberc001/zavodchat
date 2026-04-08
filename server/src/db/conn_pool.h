#ifndef DB_CONN_POOL_H
#define DB_CONN_POOL_H

#include <pqxx/pqxx>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_set>

class db_connection_pool;

struct inner_db_connection
{
	inner_db_connection(std::string conn_str);

	pqxx::connection pqc;
	std::unordered_set<std::string> prepared_statements;
};

class db_connection
{
public:
	db_connection(std::shared_ptr<inner_db_connection> conn, db_connection_pool& pool);
	~db_connection();
	pqxx::connection& operator*();

	// Returns false is statement is already prepared.
	bool prepare(std::string name, std::string query);

private:
	std::unordered_set<std::string> prepared_statements;

	std::shared_ptr<inner_db_connection> conn;
	db_connection_pool& pool;
};

class db_connection_pool
{
public:
	db_connection_pool(std::string conn_str, size_t conn_cnt = 10);
	db_connection hold();

	void release(std::shared_ptr<inner_db_connection> conn); // to be used by connection class
private:

	std::queue<std::shared_ptr<inner_db_connection>> pool;
	std::mutex q_mutex; std::condition_variable q_cond;

	// these variables are not needed to be stored, but can be used to dynamically resize connection count
	size_t conn_cnt;
	std::string conn_str;
};

#endif
