#include "resource/server.h"
#include "resource/utils.h"
#include <nlohmann/json.hpp>

#include <iostream>

server_resource::server_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_resource::render_GET(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	try{
	pqxx::result r = tx.exec("SELECT server_id, name, avatar FROM user_x_server NATURAL JOIN servers WHERE user_id = $1", pqxx::params(user_id));
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::server_json_from_row(r[i]);
	} catch(std::exception& e){
		std::cout << e.what() << "\n";
	}
	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> server_resource::render_PUT(const http_request& req)
{
	std::string_view name = req.get_arg("name");

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	pqxx::result r;
	r = tx.exec("SELECT owner_id FROM servers WHERE owner_id = $1", pqxx::params(user_id));
	if(r.size() >= owned_per_user)
		return std::shared_ptr<http_response>(new string_response("User owns more than " + std::to_string(owned_per_user) + " servers", 403));
	
	int server_id;
	try{
		r = tx.exec("INSERT INTO servers(name, owner_id) VALUES($1, $2) RETURNING server_id", pqxx::params(name, user_id));
		server_id = r[0]["server_id"].as<int>();
		tx.exec("INSERT INTO user_x_server(user_id, server_id) VALUES($1, $2)", pqxx::params(user_id, server_id));
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Server name is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response(std::to_string(server_id), 200));
}


server_id_resource::server_id_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_id_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT server_id, name, avatar FROM servers WHERE server_id = $1", pqxx::params(server_id));
	return std::shared_ptr<http_response>(new string_response(resource_utils::server_json_from_row(r[0]).dump(), 200));
}
std::shared_ptr<http_response> server_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	tx.exec("DELETE FROM servers WHERE server_id = $1", pqxx::params(server_id));
	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Deleted", 200));
}
