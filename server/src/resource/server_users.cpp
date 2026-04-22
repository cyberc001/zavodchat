#include "resource/server_users.h"
#include "resource/utils.h"
#include "resource/role_utils.h"
#include "resource/json_utils.h"
#include <unordered_map>

server_users_resource::server_users_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/servers/{server_id}/users", pool, cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> server_users_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	std::string pg_query, pg_order;
	pqxx::params pr(server_id);
	err = resource_utils::pagination_query(req, cfg, "user_id", pr, pg_query, &pg_order);
	if(err) return err;

	std::string displayname = "", where_displayname = "";
	auto args = req.get_args();
	if(args.find(std::string_view("displayname")) != args.end()){
		pr.append(std::string(args["displayname"]));
		where_displayname = "AND name LIKE '%' || $" + std::to_string(pr.size()) + " || '%'";
	}

	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status, role_id FROM user_x_server NATURAL JOIN users WHERE user_id IN (SELECT DISTINCT ON(user_id) user_id FROM user_x_server WHERE server_id = $1" + pg_query + ") AND server_id = $1 " + where_displayname + " " + pg_order, pr); // select first distinct 'count' users, then get all user_id-role_id entries for those selected users
	std::unordered_map<int, size_t> r_users; // for O(1) access to users already inserted in res to append role_ids to them
	nlohmann::json res = nlohmann::json::array();
	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		if(r_users.find(user_id) == r_users.end()){
			nlohmann::json user_json = json_utils::user_from_row(r[i]);
			user_json["roles"] = nlohmann::json::array();
			res += user_json;
			r_users[user_id] = res.size() - 1;
		}
		res[r_users[user_id]]["roles"].push_back(r[i]["role_id"].as<int>());
	}

	return create_response::string(req, res.dump(), 200);
}

server_user_id_resource::server_user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv):
	base_resource(ws, "/servers/{server_id}/users/{server_user_id}", pool,  cfg),
	sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_user_id_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_server_user_id(req, server_id, tx, server_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status, role_id FROM user_x_server NATURAL JOIN users WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	nlohmann::json res = json_utils::user_from_row(r[0]);
	res["roles"] = nlohmann::json::array();
	for(size_t i = 0; i < r.size(); ++i)
		res["roles"].push_back(r[i]["role_id"].as<int>());

	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_user_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_BAN_MEMBERS);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_server_user_id(req, server_id, tx, server_user_id);
	if(err) return err;

	if(!resource_utils::check_server_owner(req, server_user_id, server_id, tx))
		return create_response::string(req, "Owner cannot be kicked", 403);
	if(server_user_id != user_id){
		err = role_utils::check_user_lower_than_other(req, tx, server_id, user_id, server_user_id);
		if(err) return err;
	}

	tx.exec("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	tx.commit();

	socket_event ev;
	json_utils::set_ids(ev.data, server_id);
	ev.data["id"] = server_user_id;
	ev.name = "user_kicked";
	sserv.send_to_server(server_id, tx, ev);
	sserv.send_to_user(server_user_id, tx, ev);

	return create_response::string(req, "Kicked", 200);
}

server_user_role_id_resource::server_user_role_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv):
	base_resource(ws, "/servers/{server_id}/users/{server_user_id}/roles/{server_role_id}", pool, cfg),
	sserv{sserv}
{
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_user_role_id_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_server_user_id(req, server_id, tx, server_user_id);
	if(err) return err;

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_ROLES);
	if(err) return err;

	err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, server_role_id);
	if(err) return err;
	if(user_id != server_user_id){
		err = role_utils::check_user_lower_than_other(req, tx, server_id, user_id, server_user_id);
		if(err) return err;
	}

	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2 AND role_id = $3", pqxx::params(server_user_id, server_id, server_role_id));
	if(r.size())
		return create_response::string(req, "User already has that role", 202);
	tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(server_user_id, server_id, server_role_id));
	tx.commit();

	socket_event ev;
	json_utils::set_ids(ev.data, server_id);
	ev.data["user_id"] = server_user_id;
	ev.data["role_id"] = server_role_id;
	ev.name = "role_assigned";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Assigned", 200);
}
std::shared_ptr<http_response> server_user_role_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_server_user_id(req, server_id, tx, server_user_id);
	if(err) return err;

	int server_role_id;
	err = role_utils::parse_server_role_id(req, server_id, tx, server_role_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_ROLES);
	if(err) return err;

	err = role_utils::check_role_lower_than_user(req, tx, server_id, user_id, server_role_id);
	if(err) return err;
	if(user_id != server_user_id){
		err = role_utils::check_user_lower_than_other(req, tx, server_id, user_id, server_user_id);
		if(err) return err;
	}
	err = role_utils::check_role_not_default(req, tx, server_id, server_role_id);
	if(err) return err;

	tx.exec("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2 AND role_id = $3", pqxx::params(server_user_id, server_id, server_role_id));
	tx.commit();

	socket_event ev;
	json_utils::set_ids(ev.data, server_id);
	ev.data["user_id"] = server_user_id;
	ev.data["role_id"] = server_role_id;
	ev.name = "role_disallowed";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Removed", 200);
}
