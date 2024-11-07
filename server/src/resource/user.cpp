#include "resource/user.h"
#include "resource/utils.h"

#include <iostream>

user_id_resource::user_id_resource(db_connection_pool& pool, auth_resource& auth) : pool{pool}, auth{auth}
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> user_id_resource::render_GET(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = parse_id(req, tx, user_id);
	if(err) return err;

	pqxx::result r = tx.exec_params("SELECT user_id, name, avatar, status FROM users WHERE user_id = $1", user_id);
	if(!r.size())
		return std::shared_ptr<http_response>(new string_response("User with this ID doesn't exist", 404));

	return std::shared_ptr<http_response>(new string_response(resource_utils::user_json_from_row(r[0]).dump(), 200));
}

std::shared_ptr<http_response> user_id_resource::parse_id(const http_request& req, pqxx::work& tx, int& user_id)
{
	try{
		user_id = std::stoi(std::string(req.get_arg("user_id")));
	} catch(std::invalid_argument& e){
		return std::shared_ptr<http_response>(new string_response("Invalid user ID", 400));
	}
	return nullptr;
}
