#include "resource/server_bans.h"
#include "resource/utils.h"
#include "resource/role_utils.h"
#include "resource/json_utils.h"

server_bans_resource::server_bans_resource(webserver& ws, db_connection_pool& pool, const config& cfg):
	base_resource(ws, "/servers/{server_id}/bans", pool, cfg)
{
	ban_time_thr = std::thread(server_bans_resource::ban_time_func, std::ref(*this));
	set_allowing("GET", true);
}

std::shared_ptr<http_response> server_bans_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);
	
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						"perms1", PERM1_BAN_MEMBERS);
	if(err) return err;

	std::string pg_query;
	pqxx::params pr(server_id);
	err = resource_utils::pagination_query(req, cfg, "ban_id", pr, pg_query);

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT ban_id, expiration_time, user_id, name, avatar, status FROM server_bans NATURAL JOIN users WHERE server_id = $1" + pg_query, pr);
	for(size_t i = 0; i < r.size(); ++i)
		res += json_utils::ban_from_row(r[i]);

	return create_response::string(req, res.dump(), 200);
}

void server_bans_resource::ban_time_func(server_bans_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec("DELETE FROM server_bans WHERE expiration_time IS NOT NULL AND expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.cfg.cleanup_period));
	}
}


server_ban_id_resource::server_ban_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv):
	base_resource(ws, "/servers/{server_id}/bans/{server_ban_id}", pool, cfg),
	sserv{sserv}
{
	set_allowing("POST", true);
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_ban_id_resource::render_POST(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						"perms1", PERM1_BAN_MEMBERS);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_index(req, "server_ban_id", server_user_id);
	if(err) return err;
	err = resource_utils::check_server_member(req, server_user_id, server_id, tx);
	if(err) return err;

	err = role_utils::check_user_lower_than_other(req, tx, server_id, user_id, server_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT user_id FROM server_bans WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	if(r.size())
		return create_response::string(req, "Already banned", 202);

	std::string expires;
	err = resource_utils::parse_timestamp(req, "expires", expires);
	if(err) return err;

	try{
		tx.exec("INSERT INTO server_bans(user_id, server_id, expiration_time) VALUES ($1, $2, $3)", pqxx::params(server_user_id, server_id, expires.size() ? expires.c_str() : nullptr));
		r = tx.exec("SELECT ban_id, expiration_time, user_id, name, avatar, status FROM server_bans NATURAL JOIN users WHERE server_id = $1 AND user_id = $2", pqxx::params(server_id, server_user_id));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid date/time format", 400);
	}

	// delete user from server
	tx.exec("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	tx.commit();

	socket_event ev;
	ev.data = json_utils::ban_from_row(r[0]);
	json_utils::set_ids(ev.data, server_id);
	ev.name = "user_banned";
	sserv.send_to_server(server_id, tx, ev);
	sserv.send_to_user(server_user_id, tx, ev);

	return create_response::string(req, "Banned", 200);
}
std::shared_ptr<http_response> server_ban_id_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						"perms1", PERM1_BAN_MEMBERS);
	if(err) return err;

	int ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, ban_id);
	if(err) return err;

	auto args = req.get_args();
	bool changed = false;
	if(args.find(std::string_view("expires")) != args.end()){
		std::string expires;
		err = resource_utils::parse_timestamp(req, "expires", expires);
		try{
			tx.exec("UPDATE server_bans SET expiration_time = $1 WHERE ban_id = $2", pqxx::params(expires.size() ? expires.c_str() : nullptr, ban_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Invalid date/time format", 400);
		}
		changed = true;
	}
	tx.commit();

	if(changed){
		socket_event ev;
		json_utils::set_ids(ev.data, server_id);
		ev.data["id"] = ban_id;
		ev.data["expires"] = args["expires"];
		ev.name = "ban_changed";
		sserv.send_to_server(server_id, tx, ev);
	}

	return create_response::string(req, "Changed ban", 200);
}

std::shared_ptr<http_response> server_ban_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission(req, tx, server_id, user_id,
						"perms1", PERM1_BAN_MEMBERS);
	if(err) return err;

	int ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, ban_id);
	if(err) return err;

	tx.exec("DELETE FROM server_bans WHERE ban_id = $1 AND server_id = $2", pqxx::params(ban_id, server_id));
	tx.commit();

	socket_event ev;
	json_utils::set_ids(ev.data, server_id);
	ev.data["id"] = ban_id;
	ev.name = "user_unbanned";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Unbanned", 200);
}
