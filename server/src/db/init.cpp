#include "init.h"
#include <pqxx/pqxx>
#include <iostream>
#include <chrono>
#include <thread>
#include "resource/role_utils.h"
#include <cstdlib>

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

#define GET_SERVER_ID(_out, id){\
	pqxx::result r = tx.exec("SELECT server_id FROM servers WHERE name = '" id "'");\
	if(!r.size()){\
		std::cerr << "Failed to create test db: server '" id "' doesn't exist" << std::endl;\
		return;\
	}\
	_out = r[0]["server_id"].as<int>();\
}


void db_init(std::string conn_str)
{
	pqxx::connection conn{conn_str};
	{
		pqxx::work tx{conn};
		tx.exec("CREATE EXTENSION IF NOT EXISTS pgcrypto");

		tx.exec("CREATE TABLE IF NOT EXISTS users(user_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), status INTEGER NOT NULL, fs_busy BIGINT NOT NULL DEFAULT 0)");
		tx.exec("CREATE TABLE IF NOT EXISTS auth(auth_id SERIAL PRIMARY KEY, username VARCHAR(64) UNIQUE NOT NULL, password TEXT NOT NULL, user_id INTEGER REFERENCES users NOT NULL)");
		tx.exec("CREATE TABLE IF NOT EXISTS sessions(token UUID PRIMARY KEY, user_id INTEGER REFERENCES users NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");

		tx.exec("CREATE TABLE IF NOT EXISTS servers(server_id SERIAL PRIMARY KEY, name VARCHAR(64) NOT NULL, avatar VARCHAR(128), owner_id INTEGER REFERENCES users NOT NULL, default_role_id INTEGER NOT NULL DEFAULT -1, head_role_id INTEGER NOT NULL DEFAULT -1)");
		tx.exec("CREATE TABLE IF NOT EXISTS server_invites(invite_id UUID PRIMARY KEY, server_id INTEGER REFERENCES servers NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");
		tx.exec("CREATE TABLE IF NOT EXISTS server_bans(user_id INTEGER REFERENCES users ON DELETE CASCADE NOT NULL, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, expiration_time TIMESTAMP WITH TIME ZONE)");

		tx.exec("CREATE TABLE IF NOT EXISTS roles(role_id SERIAL PRIMARY KEY, prev_role_id INTEGER NOT NULL DEFAULT -1, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, name VARCHAR(64) NOT NULL, color INTEGER NOT NULL, perms1 BIGINT NOT NULL DEFAULT 0)");
		tx.exec("CREATE TABLE IF NOT EXISTS user_x_server(user_id INTEGER REFERENCES users ON DELETE CASCADE NOT NULL, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, role_id INTEGER REFERENCES roles ON DELETE CASCADE NOT NULL)"); // can be one-to-many in case a user has multiple roles

		tx.exec("CREATE TABLE IF NOT EXISTS channels(channel_id SERIAL PRIMARY KEY, server_id INTEGER REFERENCES servers ON DELETE CASCADE NOT NULL, name VARCHAR(64) NOT NULL, type INTEGER NOT NULL )");
		tx.exec("CREATE TABLE IF NOT EXISTS messages(message_id SERIAL PRIMARY KEY, channel_id INTEGER REFERENCES channels ON DELETE CASCADE NOT NULL, author_id INTEGER REFERENCES users NOT NULL, sent TIMESTAMP WITH TIME ZONE NOT NULL, last_edited TIMESTAMP WITH TIME ZONE NOT NULL, text VARCHAR(2000) )");

		tx.exec("CREATE TABLE IF NOT EXISTS fs_total_busy(total BIGINT)");


		tx.exec("CREATE INDEX IF NOT EXISTS session_user_ids ON sessions (user_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS session_expire ON sessions (expiration_time)");

		tx.exec("CREATE INDEX IF NOT EXISTS role_prev_id ON roles (prev_role_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS server_x_user ON user_x_server (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS role_x_user ON user_x_server (role_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS ban_servers ON server_bans (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS servers_owners ON servers (owner_id)");

		tx.exec("CREATE INDEX IF NOT EXISTS channels_servers ON channels (server_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS messages_channels ON messages (channel_id)");
		tx.exec("CREATE INDEX IF NOT EXISTS messages_authors ON messages (author_id)");


		tx.exec("CREATE OR REPLACE FUNCTION add_fs_total_busy() RETURNS trigger AS\n"
			"$BODY$ BEGIN\n"
			"UPDATE fs_total_busy SET total = total + (NEW.fs_busy - OLD.fs_busy);\n"
			"RETURN NEW;\n"
			"END; $BODY$\n"
			"LANGUAGE plpgsql VOLATILE");
		tx.exec("CREATE OR REPLACE TRIGGER tadd_fs_total_busy\n"
			"BEFORE UPDATE OF fs_busy ON users\n"
			"FOR EACH ROW\n"
			"EXECUTE FUNCTION add_fs_total_busy();");

		pqxx::result r = tx.exec("SELECT total FROM fs_total_busy");
		if(!r.size())
			tx.exec("INSERT INTO fs_total_busy(total) VALUES(0)");
		tx.commit();
	}
}

void db_create_test(std::string conn_str)
{
	std::cerr << "Creating test database..." << std::endl;

	pqxx::connection conn{conn_str};
	// Create 2 test users
	bool created_any_users = false;
	{
		try{
			pqxx::work tx{conn};
			pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test', 0) RETURNING user_id");
			tx.exec("INSERT INTO auth(username, password, user_id) VALUES('test', crypt('qwe123', gen_salt('bf')), $1)", pqxx::params(r[0]["user_id"].as<int>()));
			tx.commit();
			created_any_users = true;
		} catch(const pqxx::unique_violation& e){}
		try{
			pqxx::work tx{conn};
			pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test2', 0) RETURNING user_id");
			tx.exec("INSERT INTO auth(username, password, user_id) VALUES('test2', crypt('qwe123', gen_salt('bf')), $1)", pqxx::params(r[0]["user_id"].as<int>()));
			tx.commit();
			created_any_users = true;
		} catch(const pqxx::unique_violation& e){}
		try{
			pqxx::work tx{conn};
			pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES('test3', 0) RETURNING user_id");
			tx.exec("INSERT INTO auth(username, password, user_id) VALUES('test3', crypt('qwe123', gen_salt('bf')), $1)", pqxx::params(r[0]["user_id"].as<int>()));
			tx.commit();
			created_any_users = true;
		} catch(const pqxx::unique_violation& e){}
	}
	if(!created_any_users)
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
		int default_role_id_1 = role_utils::create_default_role_if_absent(tx, server_id_1);
		int test_role_id_1_1 = role_utils::insert_role(tx, server_id_1, -1, "admin", 0xFF0000);
		tx.exec("UPDATE roles SET perms1 = $1 WHERE role_id = $2", pqxx::params(65536, test_role_id_1_1));
		int test_role_id_1_2 = role_utils::insert_role(tx, server_id_1, test_role_id_1_1, "moderator", 0x0000FF);
		int test_role_id_1_3 = role_utils::insert_role(tx, server_id_1, test_role_id_1_2, "pleb", 0xFFFF00);

		r = tx.exec("SELECT user_id FROM users WHERE name = 'test2'");
		if(!r.size()){
			std::cerr << "Failed to create test server: user 'test2' doesn't exist" << std::endl;
			return;
		}
		int user_id_2 = r[0]["user_id"].as<int>();
		r = tx.exec("INSERT INTO servers(name, owner_id) VALUES('server_test2', $1) RETURNING server_id", pqxx::params(user_id_2));
		int server_id_2 = r[0]["server_id"].as<int>();
		int default_role_id_2 = role_utils::create_default_role_if_absent(tx, server_id_2);
		int test_role_id_2_1 = role_utils::insert_role(tx, server_id_2, -1, "admin", 0xFF0000);
		tx.exec("UPDATE roles SET perms1 = $1 WHERE role_id = $2", pqxx::params(65536, test_role_id_2_1));
		int test_role_id_2_2 = role_utils::insert_role(tx, server_id_2, test_role_id_2_1, "moderator", 0x0000FF);

		r = tx.exec("SELECT user_id FROM users WHERE name = 'test3'");
		if(!r.size()){
			std::cerr << "Failed to create test server: user 'test3' doesn't exist" << std::endl;
			return;
		}
		int user_id_3 = r[0]["user_id"].as<int>();

		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_1, server_id_1, default_role_id_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_1, server_id_2, default_role_id_2));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_2, server_id_1, default_role_id_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_1, server_id_1, test_role_id_1_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_1, server_id_1, test_role_id_1_3));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_2, server_id_1, test_role_id_1_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_2, server_id_1, test_role_id_1_3));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_2, server_id_1, test_role_id_1_2));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_2, server_id_2, default_role_id_2));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_3, server_id_1, default_role_id_1));
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id_3, server_id_2, default_role_id_2));

		tx.commit();
	}
	// Create 120 users for server_test
	{
		pqxx::work tx{conn};

		int server_id;
		GET_SERVER_ID(server_id, "server_test");
		int default_role_id = role_utils::find_default_role(tx, server_id);

		for(int i = 0; i < 120; ++i){
			int user_id;
			try{
				std::string username = "testuser" + std::to_string(i);
				pqxx::result r = tx.exec("INSERT INTO users(name, status) VALUES($1, $2) RETURNING user_id", pqxx::params(username, i % 4));
				user_id = r[0]["user_id"].as<int>();
				tx.exec("INSERT INTO auth(username, password, user_id) VALUES($1, crypt('qwe123', gen_salt('bf')), $2)", pqxx::params(username, user_id));
			} catch(const pqxx::unique_violation& e){}
			tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id, server_id, default_role_id));
		}
		tx.commit();
	}

	// Create a text channel and a voice channel in each test server
	{
		pqxx::work tx{conn};

		int server_id, server_id2;
		GET_SERVER_ID(server_id, "server_test");
		GET_SERVER_ID(server_id2, "server_test2");

		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test', 0)", pqxx::params(server_id));
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test_vc', 1)", pqxx::params(server_id));
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test2', 0)", pqxx::params(server_id2));
		tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, 'channel_test2_vc', 1)", pqxx::params(server_id2));

		tx.commit();
	}
	// Create 2 invites in each server
	{
		pqxx::work tx{conn};

		int server_id, server_id2;
		GET_SERVER_ID(server_id, "server_test");
		GET_SERVER_ID(server_id2, "server_test2");

		tx.exec("INSERT INTO server_invites(invite_id, server_id, expiration_time) VALUES(gen_random_uuid(), $1, $2)", pqxx::params(server_id, nullptr));
		tx.exec("INSERT INTO server_invites(invite_id, server_id, expiration_time) VALUES(gen_random_uuid(), $1, $2)", pqxx::params(server_id2, nullptr));

		tx.commit();
	}

	// Create test messages in every channel
	{
		pqxx::work tx{conn};

		pqxx::result r = tx.exec("SELECT user_id FROM users WHERE name = 'test'");
		if(!r.size()){
			std::cerr << "Failed to create test messages: user 'test' doesn't exist" << std::endl;
			return;
		}
		int user_id_1 = r[0]["user_id"].as<int>();

		r = tx.exec("SELECT user_id FROM users WHERE name = 'test2'");
		if(!r.size()){
			std::cerr << "Failed to create test messages: user 'test2' doesn't exist" << std::endl;
			return;
		}
		int user_id_2 = r[0]["user_id"].as<int>();


		const char* chan_names[] = {"channel_test", "channel_test2"};
		for(size_t i = 0; i < sizeof(chan_names) / sizeof(*chan_names); ++i)
		{
			r = tx.exec("SELECT channel_id FROM channels WHERE name = $1", pqxx::params(chan_names[i]));
			if(!r.size()){
				std::cerr << "Failed to create test messages: channel '" << chan_names[i] << "' doesn't exist" << std::endl;
				return;
			}

			int chan_id = r[0]["channel_id"].as<int>();
			for(size_t j = 0; j < 120; ++j){
				std::string text(rand() % 124 + 4, ' ');
				for(size_t k = 0; k < text.size(); ++k)
					text[k] = rand() % ('}' - '!' + 1) + '!';
				for(int k = 0; k < rand() % 4; ++k)
					text[rand() % text.size()] = '\n';
				tx.exec("INSERT INTO messages(channel_id, author_id, sent, last_edited, text) VALUES($1, $2, date_subtract(now(), ($3 || ' minute')::interval), date_subtract(now(), ($4 || ' minute')::interval), $5)", pqxx::params(chan_id, (rand() & 0x1) ? user_id_1 : user_id_2, (int)((120 - j) * 24), (int)((120 - j) * 24 + rand() % 3), text));
				
			}
			tx.commit();
		}
	}

	std::cerr << "Created test database" << std::endl;

}
