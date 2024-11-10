#include "auth.h"
#include <iostream>
#include <limits>

auth_resource::auth_resource(db_connection_pool& pool): pool{pool}
{
	rand_gen = std::mt19937{seed_gen()};
	rand_distribution = std::uniform_int_distribution<session_token>{0, std::numeric_limits<session_token>::max()};

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

	session_token st = generate_session_token();
	sessions[st] = user_id;
	return std::shared_ptr<http_response>(new string_response(std::to_string(st), 200));
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
	session_token token;
	auto err = parse_session_token(req, token);
	if(err) return err;
	int user_id = sessions[token];

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

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

session_token auth_resource::generate_session_token()
{
	return rand_distribution(rand_gen);
}
std::shared_ptr<http_response> auth_resource::parse_session_token(const http_request& req, session_token& st)
{
	try{
		st = std::stoull(std::string(req.get_header("token")));
	} catch(std::invalid_argument& e){
		return std::shared_ptr<http_response>(new string_response("Invalid token", 400));
	}
	if(sessions.find(st) == sessions.end())
		return std::shared_ptr<http_response>(new string_response("Expired or invalid token", 401));
	return std::shared_ptr<http_response>(nullptr);
}
