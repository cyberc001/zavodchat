#include "auth.h"
#include <iostream>
#include <limits>

auth_resource::auth_resource(connection_pool& pool): pool{pool}
{
	rand_gen = std::mt19937{seed_gen()};
	rand_distribution = std::uniform_int_distribution<session_token>{0, std::numeric_limits<session_token>::max()};
}

std::shared_ptr<http_response> auth_resource::render_POST(const http_request& req)
{
	std::string_view username = req.get_header("username"),
			password = req.get_header("password");

	auto conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r = tx.exec_params("SELECT user_id, username, password FROM auth WHERE username = $1", username);
	if(r.size() != 1)
		return std::shared_ptr<http_response>(new string_response("Invalid username or password", 404));
	std::string db_hash = r[0]["password"].as<std::string>();
	int user_id = r[0]["user_id"].as<int>();

	r = tx.exec_params("SELECT crypt($1, $2)", password, db_hash);
	std::string password_hash = r[0][0].as<std::string>();
	pool.release(conn);

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

	auto conn = pool.hold();
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
