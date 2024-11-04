#include "resource/server.h"
#include <nlohmann/json.hpp>

#include <iostream>

server_resource::server_resource(connection_pool& pool, auth_resource& auth): pool{pool}, auth{auth}
{}

std::shared_ptr<http_response> server_resource::render_GET(const http_request& req)
{
	session_token token;
	auto err = auth.parse_session_token(req, token);
	if(err) return err;
	int user_id = auth.sessions[token];

	auto conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r = tx.exec_params("SELECT server_id FROM user_x_server WHERE user_id = $1", user_id);
	nlohmann::json res;
	for(size_t i = 0; i < r.size(); ++i)
		res += r[i]["server_id"].as<int>();
	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> server_resource::render_POST(const http_request& req)
{
	std::string_view name = req.get_header("name");
	session_token token;
	auto err = auth.parse_session_token(req, token);
	if(err) return err;
	int user_id = auth.sessions[token];

	auto conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r;
	r = tx.exec_params("SELECT owner_id FROM servers WHERE owner_id = $1", user_id);
	if(r.size() >= owned_per_user)
		return std::shared_ptr<http_response>(new string_response("Too much servers owned by user", 403));
	
	int server_id;
	try{
		r = tx.exec_params("INSERT INTO servers(name, owner_id) VALUES($1, $2) RETURNING server_id", name, user_id);
		server_id = r[0]["server_id"].as<int>();
		tx.exec_params("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", user_id, server_id);
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Server name is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response(std::to_string(server_id), 200));
}
