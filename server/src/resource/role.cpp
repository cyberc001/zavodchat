#include "resource/role.h"

#include "resource/utils.h"
#include "resource/role_utils.h"

server_roles_resource::server_roles_resource(db_connection_pool& pool, socket_main_server& sserv) : base_resource(), pool{pool}, sserv{sserv}
{
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

	err = role_utils::check_permission1(tx, server_id, user_id, PERM1_MANAGE_ROLES);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT role_id FROM roles WHERE server_id = $1", pqxx::params(server_id));
	if(r.size() >= max_per_server)
		return create_response::string("Server has more than " + std::to_string(max_per_server) + " roles", 403);

	int next_role_id;
	try{
		next_role_id = std::stoi(std::string(req.get_arg("next_role_id")));
	} catch(std::invalid_argument& e){
		return create_response::string("Invalid next_role_id: '" + std::string(req.get_arg("next_role_id")) + "'", 400);
	}
	err = role_utils::check_server_role(next_role_id, server_id, tx);
	if(err) return err;
	err = role_utils::check_role_lower_than_user(tx, server_id, user_id, next_role_id, true);
	if(err) return err;
	err = role_utils::check_role_not_default(tx, server_id, next_role_id);
	if(err) return err;

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

	long long perms1 = 0;
	auto args = req.get_args();
	if(args.find(std::string_view("perms1")) != args.end()){
		try{
			perms1 = std::stoll(std::string(req.get_arg("perms1")));
		} catch(std::invalid_argument& e){
			return create_response::string("Invalid perms1: '" + std::string(req.get_arg("perms1")) + "'", 400);
		}
		err = role_utils::check_validity_perms1(perms1);
		if(err) return err;
	}

	int inserted_id = role_utils::insert_role(tx, server_id, next_role_id, name, color, perms1);
	if(inserted_id == -1)
		return create_response::string("next_role_id does not exist", 400);
	if(inserted_id == -2)
		return create_response::string("name is too long", 400);

	tx.commit();

	socket_event ev;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.data["id"] = inserted_id;
	ev.data["name"] = name;
	ev.data["color"] = color;
	ev.data["perms1"] = perms1;
	ev.name = "role_created";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(std::to_string(inserted_id), 200);
}


server_role_id_resource::server_role_id_resource(db_connection_pool& pool, socket_main_server& sserv) : base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_role_id_resource::render_POST(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(tx, server_id, user_id, PERM1_MANAGE_ROLES);
	if(err) return err;

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_role_lower_than_user(tx, server_id, user_id, server_role_id);
	if(err) return err;

	socket_event ev;
	ev.data["id"] = server_role_id;
	resource_utils::json_set_ids(ev.data, server_id);

	// Parse optional arguments
	auto args = req.get_args();

	if(args.find(std::string_view("next_role_id")) != args.end()){
		err = role_utils::check_role_not_default(tx, server_id, server_role_id);
		if(err) return err;

		int next_role_id;
		try{
			next_role_id = std::stoi(std::string(req.get_arg("next_role_id")));
		} catch(std::invalid_argument& e){
			return create_response::string("Invalid next_role_id: '" + std::string(req.get_arg("next_role_id")) + "'", 400);
		}
		err = role_utils::check_server_role(next_role_id, server_id, tx);
		if(err) return err;
		err = role_utils::check_role_lower_than_user(tx, server_id, user_id, next_role_id, true);
		if(err) return err;
		err = role_utils::check_role_not_default(tx, server_id, next_role_id);
		if(err) return err;

		ev.data["next_role_id"] = next_role_id;

		role_utils::move_role(tx, server_id, server_role_id, next_role_id);
	}
	if(args.find(std::string_view("name")) != args.end()){
		std::string name = req.get_arg("name");
		if(!name.size())
			return create_response::string("Empty role name", 400);

		try{
			tx.exec("UPDATE roles SET name = $1 WHERE role_id = $2", pqxx::params(name, server_role_id));
		} catch(pqxx::data_exception& e){
			return create_response::string("name is too long", 400);
		}

		ev.data["name"] = name;
	}
	if(args.find(std::string_view("color")) != args.end()){
		int color;
		try{
			color = std::stoi(std::string(req.get_arg("color")));
		} catch(std::invalid_argument& e){
			return create_response::string("Invalid role color: '" + std::string(req.get_arg("next_role_id")) + "'", 400);
		}
		if(color > 0xFFFFFF || color < 0)
			return create_response::string("Invalid role color: '" + std::string(req.get_arg("next_role_id")) + "'", 400);

		ev.data["color"] = color;

		tx.exec("UPDATE roles SET color = $1 WHERE role_id = $2", pqxx::params(color, server_role_id));
	}
	if(args.find(std::string_view("perms1")) != args.end()){
		long long perms1;
		try{
			perms1 = std::stoll(std::string(req.get_arg("perms1")));
		} catch(std::invalid_argument& e){
			return create_response::string("Invalid perms1: '" + std::string(req.get_arg("perms1")) + "'", 400);
		}
		if(role_utils::find_default_role(tx, server_role_id) == server_role_id)
			err = role_utils::check_default_validity_perms1(perms1);
		else
			err = role_utils::check_validity_perms1(perms1);
		if(err) return err;

		ev.data["perms1"] = perms1;

		tx.exec("UPDATE roles SET perms1 = $1 WHERE role_id = $2", pqxx::params(perms1, server_role_id));
	}

	tx.commit();

	ev.name = "role_changed";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string("Changed", 200);
}

std::shared_ptr<http_response> server_role_id_resource::render_DELETE(const http_request& req)
{	
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(tx, server_id, user_id, PERM1_MANAGE_ROLES);
	if(err) return err;

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_role_lower_than_user(tx, server_id, user_id, server_role_id);
	if(err) return err;
	err = role_utils::check_role_not_default(tx, server_id, server_role_id);
	if(err) return err;

	role_utils::delete_role(tx, server_id, server_role_id);
	tx.commit();

	socket_event ev;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.data["id"] = server_role_id;
	ev.name = "role_deleted";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string("Deleted", 200);
}
