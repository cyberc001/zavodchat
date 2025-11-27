#include "resource/server.h"
#include "resource/utils.h"
#include "resource/file_utils.h"
#include "resource/role_utils.h"
#include <nlohmann/json.hpp>

server_resource::server_resource(db_connection_pool& pool): base_resource(), pool{pool}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> server_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT distinct server_id, name, avatar FROM user_x_server NATURAL JOIN servers WHERE user_id = $1", pqxx::params(user_id));
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::server_json_from_row(r[i]);
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	std::string_view name = req.get_arg("name");

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	pqxx::result r;
	r = tx.exec("SELECT owner_id FROM servers WHERE owner_id = $1", pqxx::params(user_id));
	if(r.size() >= owned_per_user)
		return create_response::string(req, "User owns more than " + std::to_string(owned_per_user) + " servers", 403);
	
	int server_id;
	try{
		r = tx.exec("INSERT INTO servers(name, owner_id) VALUES($1, $2) RETURNING server_id", pqxx::params(name, user_id));
		server_id = r[0]["server_id"].as<int>();
		int default_role_id = role_utils::create_default_role_if_absent(tx, server_id);
		tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id, server_id, default_role_id));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Server name is too long", 400);
	}
	tx.commit();

	return create_response::string(req, std::to_string(server_id), 200);
}


server_id_resource::server_id_resource(db_connection_pool& pool, socket_main_server& sserv): base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_id_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT server_id, name, avatar FROM servers WHERE server_id = $1", pqxx::params(server_id));
	return create_response::string(req, resource_utils::server_json_from_row(r[0]).dump(), 200);
}
std::shared_ptr<http_response> server_id_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_CHANGE_SERVER);
	if(err) return err;

	bool updated = false;
	auto args = req.get_args();
	if(args.find(std::string_view("name")) != args.end()){
		std::string name = std::string(req.get_arg("name"));
		try{
			tx.exec("UPDATE servers SET name = $1 WHERE server_id = $2", pqxx::params(name, server_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Server name is too long", 400);
		}
		updated = true;
	}
	if(args.find(std::string_view("owner_id")) != args.end()){
		int new_owner_id;
		err = resource_utils::parse_index(req, "owner_id", new_owner_id);
		if(err) return err;

		if(new_owner_id == user_id)
			return create_response::string(req, "User is already an owner", 202);

		pqxx::result r = tx.exec("SELECT owner_id FROM servers WHERE owner_id = $1", pqxx::params(new_owner_id));
		if(r.size() >= owned_per_user)
			return create_response::string(req, "User owns more than " + std::to_string(owned_per_user) + " servers", 403);


		err = resource_utils::check_server_member(req, new_owner_id, server_id, tx);
		if(err) return err;
		tx.exec("UPDATE servers SET owner_id = $1 WHERE server_id = $2", pqxx::params(new_owner_id, server_id));

		socket_event ev;
		ev.data["id"] = server_id;
		ev.name = "got_server_owner";
		sserv.send_to_user(new_owner_id, tx, ev);

		updated = true;
	}
	if(args.find(std::string_view("avatar")) != args.end()){
		std::string fname;
		err = file_utils::parse_server_avatar(req, "avatar", user_id, fname);
		if(err)
			return err;
		tx.exec("UPDATE servers SET avatar = $1 WHERE server_id = $2", pqxx::params(fname, server_id));

		updated = true;
	}
	tx.commit();

	if(updated){
		socket_event ev;
		pqxx::result r = tx.exec("SELECT server_id, name, avatar FROM servers WHERE server_id = $1", pqxx::params(server_id));
		ev.data = resource_utils::server_json_from_row(r[0]);
		ev.name = "server_edited";
		sserv.send_to_server(server_id, tx, ev);
	}

	return create_response::string(req, "Changed", 200);
}
std::shared_ptr<http_response> server_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(req, user_id, server_id, tx);
	if(err) return err;

	socket_event ev;
	ev.data["id"] = server_id;
	ev.name = "server_deleted";
	sserv.send_to_server(server_id, tx, ev);

	tx.exec("DELETE FROM servers WHERE server_id = $1", pqxx::params(server_id));
	tx.commit();

	return create_response::string(req, "Deleted", 200);
}
