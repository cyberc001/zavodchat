#include "resource/server.h"

#include <iostream>

server_resource::server_resource(connection_pool& pool, auth_resource& auth): pool{pool}, auth{auth}
{}

std::shared_ptr<http_response> server_resource::render_POST(const http_request& req)
{
	std::string_view token_s = req.get_header("token"),
			name = req.get_header("name");
	session_token token;
	PARSE_SESSION_TOKEN(token, std::string(token_s));
	int user_id = auth.sessions[token];

	auto conn = pool.hold();
	pqxx::work tx{*conn};

	pqxx::result r;
	r = tx.exec_params("SELECT owner_id FROM servers WHERE owner_id = $1", user_id);
	if(r.size() >= owned_per_user)
		return std::shared_ptr<http_response>(new string_response("Too much servers owned by user", 403));
	
	try{
		r = tx.exec_params("INSERT INTO servers(name, owner_id) VALUES($1, $2) RETURNING server_id", name, user_id);
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Server name is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response(std::to_string(r[0]["server_id"].as<int>()), 200));
}
