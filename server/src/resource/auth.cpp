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
	std::string_view username = req.get_header("username"),
			password = req.get_header("password");

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r = tx.exec_params("SELECT user_id, username, password FROM auth WHERE username = $1", username);
	if(r.size() != 1)
		return std::shared_ptr<http_response>(new string_response("Invalid username or password", 404));
	std::string db_hash = r[0]["password"].as<std::string>();
	int user_id = r[0]["user_id"].as<int>();

	r = tx.exec_params("SELECT crypt($1, $2)", password, db_hash);
	std::string password_hash = r[0][0].as<std::string>();

	if(db_hash != password_hash)
		return std::shared_ptr<http_response>(new string_response("Invalid username or password", 404));

	session_token token = create_session(user_id, tx);
	return std::shared_ptr<http_response>(new string_response(token, 200));
}
std::shared_ptr<http_response> auth_resource::render_PUT(const http_request& req)
{
	std::string_view username = req.get_header("username"),
			displayname = req.get_header("displayname"),
			password = req.get_header("password");

	if(username.size() < min_username_length)
		return std::shared_ptr<http_response>(new string_response("Username is shorter than " + std::to_string(min_username_length) + " characters", 400));
	if(password.size() < min_password_length)
		return std::shared_ptr<http_response>(new string_response("Password is shorter than " + std::to_string(min_password_length) + " characters", 400));

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r;
	try{
		r = tx.exec_params("INSERT INTO users(name, status) VALUES($1, 0) RETURNING user_id", displayname);
	} catch(const pqxx::unique_violation& e){
		return std::shared_ptr<http_response>(new string_response("Displayname already exists", 403));
	}

	try{
		tx.exec_params("INSERT INTO auth(username, password, user_id) VALUES($1, crypt($2, gen_salt('bf')), $3)", username, password, r[0]["user_id"].as<int>());
	} catch(const pqxx::unique_violation& e){
		return std::shared_ptr<http_response>(new string_response("Username already exists", 403));
	}

	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Registered", 200));
}
std::shared_ptr<http_response> auth_resource::render_POST(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = parse_session_token(req, tx, user_id);
	if(err) return err;

	auto hdrs = req.get_headers();
	if(hdrs.find("username") != hdrs.end()){
		std::string username = std::string(req.get_header("username"));
		if(username.size() < min_username_length)
			return std::shared_ptr<http_response>(new string_response("Username is shorter than " + std::to_string(min_username_length) + " characters", 400));
		try{
			tx.exec_params("UPDATE auth SET username = $1 WHERE user_id = $2", username, user_id);
		} catch(pqxx::data_exception& e){
			return std::shared_ptr<http_response>(new string_response("Username is too long", 400));
		} catch(const pqxx::unique_violation& e){
			return std::shared_ptr<http_response>(new string_response("Username already exists", 403));
		}
	}
	if(hdrs.find("password") != hdrs.end()){
		std::string password = std::string(req.get_header("password"));
		if(password.size() < min_password_length)
			return std::shared_ptr<http_response>(new string_response("Password is shorter than " + std::to_string(min_password_length) + " characters", 400));

		try{
			tx.exec_params("UPDATE auth SET password = crypt($1, gen_salt('bf')) WHERE user_id = $2", password, user_id);
		} catch(pqxx::data_exception& e){
			return std::shared_ptr<http_response>(new string_response("Password is too long", 400));
		}
	}
	if(hdrs.find("displayname") != hdrs.end()){
		std::string displayname = std::string(req.get_header("displayname"));
		try{
			tx.exec_params("UPDATE auth SET displayname = $1 WHERE user_id = $2", displayname, user_id);
		} catch(pqxx::data_exception& e){
			return std::shared_ptr<http_response>(new string_response("Displayname is too long", 400));
		} catch(const pqxx::unique_violation& e){
			return std::shared_ptr<http_response>(new string_response("Displayname already exists", 403));
		}
	}

	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Changed", 200));
}

std::shared_ptr<http_response> auth_resource::parse_session_token(const http_request& req, pqxx::work& tx, int& user_id)
{
	pqxx::result r = tx.exec_params("SELECT user_id FROM sessions WHERE token = $1", req.get_header("token"));
	if(!r.size())
		return std::shared_ptr<http_response>(new string_response("Expired or invalid token", 401));
	user_id = r[0]["user_id"].as<int>();
	return std::shared_ptr<http_response>(nullptr);
}

session_token auth_resource::create_session(int user_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec_params("SELECT user_id FROM sessions WHERE user_id = $1", user_id);
	if(r.size()){
		r = tx.exec_params("UPDATE sessions SET token = gen_random_uuid(), expiration_time = now() + ($1 * interval '1 second') WHERE user_id = $2 RETURNING token", session_lifetime, user_id);
	} else{
		r = tx.exec_params("INSERT INTO sessions(token, user_id, expiration_time) VALUES(gen_random_uuid(), $1, now() + ($2 * interval '1 second')) RETURNING token", user_id, session_lifetime);
	}
	tx.commit();
	return r[0]["token"].as<session_token>();
}

void auth_resource::session_time_func(auth_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec_params("DELETE FROM sessions WHERE expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.session_removal_period));
	}
}
