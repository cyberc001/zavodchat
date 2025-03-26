#include "resource/role.h"

#include "resource/utils.h"
#include "resource/role_utils.h"

server_roles_resource::server_roles_resource(db_connection_pool& pool, socket_main_server& sserv) : pool{pool}, sserv{sserv}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_roles_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	std::vector<pqxx::row> r = role_utils::get_role_list(tx, server_id);
	for(size_t i = 0; i < r.size(); ++i)
		res += role_utils::role_json_from_row(r[i]);

	return create_response::string(res.dump(), 200);
}
std::shared_ptr<http_response> server_roles_resource::render_PUT(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int next_role_id;
	try{
		next_role_id = std::stoi(std::string(req.get_arg("next_role_id")));
	} catch(std::invalid_argument& e){
		return create_response::string("Invalid next_role_id: '" + std::string(req.get_arg("next_role_id")) + "'", 400);
	}
	if(next_role_id != -1){
		err = role_utils::check_server_role(next_role_id, server_id, tx);
		if(err) return err;
	}

	std::string name = req.get_arg("name");
	if(!name.size())
		return create_response::string("Empty role name", 400);
	int color;
	try{
		color = std::stoi(std::string(req.get_arg("color")));
	} catch(std::invalid_argument& e){
		return create_response::string("Invalid role color: '" + std::string(req.get_arg("next_role_id")) + "'", 400);
	}
	if(color > 0xFFFFFF || color < 0)
		return create_response::string("Invalid role color: '" + std::string(req.get_arg("next_role_id")) + "'", 400);

	int inserted_id = role_utils::insert_role(tx, server_id, next_role_id, name, color);
	if(inserted_id == -1)
		return create_response::string("next_role_id does not exist", 400);

	tx.commit();
	return create_response::string(std::to_string(inserted_id), 200);
}
