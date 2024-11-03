#include "init.h"
#include <pqxx/pqxx>

void db_init(std::string conn_str)
{
	pqxx::connection conn{conn_str};

	{
		pqxx::work tx{conn};
		tx.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto");

		tx.exec("CREATE TABLE IF NOT EXISTS users( user_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), status INTEGER NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS auth( auth_id SERIAL PRIMARY KEY, username VARCHAR(64) UNIQUE NOT NULL, password TEXT NOT NULL, user_id INTEGER REFERENCES users NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS servers( server_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), owner_id INTEGER REFERENCES users NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS user_x_server( user_id INTEGER REFERENCES users NOT NULL, server_id INTEGER REFERENCES servers NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS channels( channel_id SERIAL PRIMARY KEY, server_id INTEGER REFERENCES servers NOT NULL, name VARCHAR(64) NOT NULL, type INTEGER NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS messages( message_id SERIAL PRIMARY KEY, channel_id INTEGER REFERENCES channels NOT NULL, author_id INTEGER REFERENCES users NOT NULL, sent TIMESTAMP WITH TIME ZONE NOT NULL, last_edited TIMESTAMP WITH TIME ZONE NOT NULL, text VARCHAR(2000) )");

		tx.exec("CREATE INDEX IF NOT EXISTS servers_owners ON servers (owner_id)");

		tx.commit();
	}

	// Create a test user
	{
		pqxx::work tx{conn};
		pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test', 0) RETURNING user_id");
		try{
			tx.exec_params("INSERT INTO auth(username, password, user_id) VALUES('test', crypt('qwe123', gen_salt('bf')), $1)", r[0]["user_id"].as<int>());
			tx.commit();
		} catch(const pqxx::unique_violation& e){}
	}
}
