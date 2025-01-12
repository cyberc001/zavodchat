#include "init.h"
#include <pqxx/pqxx>
#include <iostream>
#include <chrono>
#include <thread>

void db_connect(std::string conn_str)
{
start:
	try{
		pqxx::connection conn{conn_str};
	} catch(pqxx::broken_connection& e){
		std::cerr << e.what() << "\nRetrying to connect...\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		goto start;
	}
}

void db_init(std::string conn_str)
{
	pqxx::connection conn{conn_str};

	{
		pqxx::work tx{conn};
		tx.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto");

		tx.exec("CREATE TABLE IF NOT EXISTS users(user_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), status INTEGER NOT NULL)");
		tx.exec("CREATE TABLE IF NOT EXISTS auth(auth_id SERIAL PRIMARY KEY, username VARCHAR(64) UNIQUE NOT NULL, password TEXT NOT NULL, user_id INTEGER REFERENCES users NOT NULL)");
		tx.exec("CREATE TABLE IF NOT EXISTS sessions(token UUID PRIMARY KEY, user_id INTEGER REFERENCES users NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");

		tx.exec("CREATE TABLE IF NOT EXISTS servers(server_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), owner_id INTEGER REFERENCES users NOT NULL)");
		tx.exec("CREATE TABLE IF NOT EXISTS server_invites(invite_id UUID PRIMARY KEY, server_id INTEGER REFERENCES servers NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");
		tx.exec("CREATE TABLE IF NOT EXISTS server_bans(user_id INTEGER REFERENCES users ON DELETE CASCADE NOT NULL, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");
		tx.exec("CREATE TABLE IF NOT EXISTS user_x_server(user_id INTEGER REFERENCES users ON DELETE CASCADE NOT NULL, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL)");

		tx.exec("CREATE TABLE IF NOT EXISTS channels(channel_id SERIAL PRIMARY KEY, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, name VARCHAR(64) NOT NULL, type INTEGER NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS messages(message_id SERIAL PRIMARY KEY, channel_id INTEGER REFERENCES channels NOT NULL, author_id INTEGER REFERENCES users NOT NULL, sent TIMESTAMP WITH TIME ZONE NOT NULL, last_edited TIMESTAMP WITH TIME ZONE NOT NULL, text VARCHAR(2000) )");


		tx.exec("CREATE INDEX IF NOT EXISTS session_user_ids ON sessions (user_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS session_expire ON sessions (expiration_time)");

		tx.exec("CREATE INDEX IF NOT EXISTS server_x_user ON user_x_server (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS ban_servers ON server_bans (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS servers_owners ON servers (owner_id)");

		tx.exec("CREATE INDEX IF NOT EXISTS channels_servers ON channels (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS messages_channels ON messages (channel_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS messages_authors ON messages (author_id)");

		tx.commit();
	}

	// Create 2 test users
	bool created_user1 = true, created_user2 = true;
	{
		try{
			pqxx::work tx{conn};
			pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test', 0) RETURNING user_id");
			tx.exec("INSERT INTO auth(username, password, user_id) VALUES('test', crypt('qwe123', gen_salt('bf')), $1)", pqxx::params(r[0]["user_id"].as<int>()));
			tx.commit();
		} catch(const pqxx::unique_violation& e){ created_user1 = false; }
		try{
			pqxx::work tx{conn};
			pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test2', 0) RETURNING user_id");
			tx.exec("INSERT INTO auth(username, password, user_id) VALUES('test2', crypt('qwe123', gen_salt('bf')), $1)", pqxx::params(r[0]["user_id"].as<int>()));
			tx.commit();
		} catch(const pqxx::unique_violation& e){ created_user2 = false; }
	}
	if(!created_user1 && !created_user2)
		return;
	// Create 2 test servers
	{
		pqxx::work tx{conn};

		pqxx::result r = tx.exec("SELECT user_id FROM users WHERE name = 'test'");
		if(!r.size()){
			std::cerr << "Failed to create test server: user 'test' doesn't exist" << std::endl;
			return;
		}
		int user_id_1 = r[0]["user_id"].as<int>();
		r = tx.exec("INSERT INTO servers(name, owner_id) VALUES('server_test', $1) RETURNING server_id", pqxx::params(user_id_1));
		int server_id_1 = r[0]["server_id"].as<int>();

		r = tx.exec("SELECT user_id FROM users WHERE name = 'test2'");
		if(!r.size()){
			std::cerr << "Failed to create test server: user 'test2' doesn't exist" << std::endl;
			return;
		}
		int user_id_2 = r[0]["user_id"].as<int>();
		r = tx.exec("INSERT INTO servers(name, owner_id) VALUES('server_test2', $1) RETURNING server_id", pqxx::params(user_id_2));
		int server_id_2 = r[0]["server_id"].as<int>();

		tx.exec("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", pqxx::params(user_id_1, server_id_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", pqxx::params(user_id_1, server_id_2));
		tx.exec("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", pqxx::params(user_id_2, server_id_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", pqxx::params(user_id_2, server_id_2));
		tx.commit();
	}
	// Create 2 text channels in each test server
	{
		pqxx::work tx{conn};

		pqxx::result r = tx.exec("SELECT server_id FROM servers WHERE name = 'server_test'");
		if(!r.size()){
			std::cerr << "Failed to create test channel: server 'server_test' doesn't exist" << std::endl;
			return;
		}
		int server_id = r[0]["server_id"].as<int>();
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test', 0)", pqxx::params(server_id));
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test2', 0)", pqxx::params(server_id));

		r = tx.exec("SELECT server_id FROM servers WHERE name = 'server_test2'");
		if(!r.size()){
			std::cerr << "Failed to create test channel: server 'server_test2' doesn't exist" << std::endl;
			return;
		}
		server_id = r[0]["server_id"].as<int>();
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test', 0)", pqxx::params(server_id));
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test2', 0)", pqxx::params(server_id));

		tx.commit();
	}
}
