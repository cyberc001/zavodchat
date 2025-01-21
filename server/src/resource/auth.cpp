#include "auth.h"
#include <iostream>
#include <limits>
#include <resource/utils.h>

auth_resource::auth_resource(db_connection_pool& pool): pool{pool}
{
	session_time_thr = std::thread(auth_resource::session_time_func, std::ref(*this));

	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> auth_resource::render_GET(const http_request& req)
{
	std::string_view username = req.get_arg("username"),
			password = req.get_arg("password");

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r = tx.exec("SELECT user_id, username, password FROM auth WHERE username = $1", pqxx::params(username));
	if(r.size() != 1)
		return create_response::string("Invalid username or password", 404);
	std::string db_hash = r[0]["password"].as<std::string>();
	int user_id = r[0]["user_id"].as<int>();

	r = tx.exec("SELECT crypt($1, $2)", pqxx::params(password, db_hash));
	std::string password_hash = r[0][0].as<std::string>();

	if(db_hash != password_hash)
		return create_response::string("Invalid username or password", 404);

	session_token token = create_session(user_id, tx);
	return create_response::string(token, 200);
}
std::shared_ptr<http_response> auth_resource::render_PUT(const http_request& req)
{
	std::string_view username = req.get_arg("username"),
			displayname = req.get_arg("displayname"),
			password = req.get_arg("password");

	if(username.size() < min_username_length)
		return create_response::string("Username is shorter than " + std::to_string(min_username_length) + " characters", 400);
	if(password.size() < min_password_length)
		return create_response::string("Password is shorter than " + std::to_string(min_password_length) + " characters", 400);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r;
	try{
		r = tx.exec("INSERT INTO users(name, status) VALUES($1, 0) RETURNING user_id", pqxx::params(displayname));
	} catch(const pqxx::unique_violation& e){
		return create_response::string("Displayname already exists", 403);
	}

	try{
		tx.exec("INSERT INTO auth(username, password, user_id) VALUES($1, crypt($2, gen_salt('bf')), $3)", pqxx::params(username, password, r[0]["user_id"].as<int>()));
	} catch(const pqxx::unique_violation& e){
		return create_response::string("Username already exists", 403);
	}

	tx.commit();
	return create_response::string("Registered", 200);
}
std::shared_ptr<http_response> auth_resource::render_POST(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	auto args = req.get_args();
	if(args.find(std::string_view("username")) != args.end()){
		std::string username = std::string(req.get_arg("username"));
		if(username.size() < min_username_length)
			return create_response::string("Username is shorter than " + std::to_string(min_username_length) + " characters", 400);
		try{
			tx.exec("UPDATE auth SET username = $1 WHERE user_id = $2", pqxx::params(username, user_id));
		} catch(pqxx::data_exception& e){
			return create_response::string("Username is too long", 400);
		} catch(const pqxx::unique_violation& e){
			return create_response::string("Username already exists", 403);
		}
	}
	if(args.find(std::string_view("password")) != args.end()){
		std::string password = std::string(req.get_arg("password"));
		if(password.size() < min_password_length)
			return create_response::string("Password is shorter than " + std::to_string(min_password_length) + " characters", 400);

		try{
			tx.exec("UPDATE auth SET password = crypt($1, gen_salt('bf')) WHERE user_id = $2", pqxx::params(password, user_id));
		} catch(pqxx::data_exception& e){
			return create_response::string("Password is too long", 400);
		}
	}
	if(args.find(std::string_view("displayname")) != args.end()){
		std::string displayname = std::string(req.get_arg("displayname"));
		try{
			tx.exec("UPDATE auth SET displayname = $1 WHERE user_id = $2", pqxx::params(displayname, user_id));
		} catch(pqxx::data_exception& e){
			return create_response::string("Displayname is too long", 400);
		} catch(const pqxx::unique_violation& e){
			return create_response::string("Displayname already exists", 403);
		}
	}

	tx.commit();
	return create_response::string("Changed", 200);
}

session_token auth_resource::create_session(int user_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT user_id FROM sessions WHERE user_id = $1", pqxx::params(user_id));
	if(r.size()){
		r = tx.exec("UPDATE sessions SET token = gen_random_uuid(), expiration_time = now() + ($1 * interval '1 second') WHERE user_id = $2 RETURNING token", pqxx::params(session_lifetime, user_id));
	} else{
		r = tx.exec("INSERT INTO sessions(token, user_id, expiration_time) VALUES(gen_random_uuid(), $1, now() + ($2 * interval '1 second')) RETURNING token", pqxx::params(user_id, session_lifetime));
	}
	tx.commit();
	return r[0]["token"].as<session_token>();
}

void auth_resource::session_time_func(auth_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec("DELETE FROM sessions WHERE expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.cleanup_period));
	}
}
