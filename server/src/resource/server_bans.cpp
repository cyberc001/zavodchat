#include "resource/server_bans.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

server_bans_resource::server_bans_resource(db_connection_pool& pool) : base_resource(), pool{pool}
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

	err = resource_utils::check_server_owner(req, user_id, server_id, tx);
	if(err) return err;

	int start;
	err = resource_utils::parse_index(req, "start", start, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, max_get_count);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status FROM server_bans NATURAL JOIN users WHERE server_id = $1 LIMIT $2 OFFSET $3", pqxx::params(server_id, count, start));
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::user_json_from_row(r[i]);

	return create_response::string(req, res.dump(), 200);
}

void server_bans_resource::ban_time_func(server_bans_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec("DELETE FROM server_bans WHERE expiration_time IS NOT NULL AND expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.cleanup_period));
	}
}


server_ban_id_resource::server_ban_id_resource(db_connection_pool& pool, socket_main_server& sserv) : base_resource(), pool{pool}, sserv{sserv}
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

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_BAN_MEMBERS);
	if(err) return err;

	int server_ban_id;
	err = resource_utils::parse_index(req, "server_ban_id", server_ban_id);
	if(err) return err;
	err = resource_utils::check_server_member(req, server_ban_id, server_id, tx);
	if(err) return err;

	err = role_utils::check_user_lower_than_other(req, tx, server_id, user_id, server_ban_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT user_id FROM server_bans WHERE user_id = $1 AND server_id = $2", pqxx::params(server_ban_id, server_id));
	if(r.size())
		return create_response::string(req, "Already banned", 202);

	std::string expires;
	err = resource_utils::parse_timestamp(req, "expires", expires);
	if(err) return err;

	try{
		tx.exec("INSERT INTO server_bans(user_id, server_id, expiration_time) VALUES ($1, $2, $3)", pqxx::params(server_ban_id, server_id, expires.size() ? expires.c_str() : nullptr));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid date/time format", 400);
	}

	socket_event ev;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.data["id"] = server_ban_id;
	ev.name = "user_left";
	sserv.send_to_server(server_id, tx, ev);

	// delete user from server
	tx.exec("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(server_ban_id, server_id));
	tx.commit();

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

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_BAN_MEMBERS);
	if(err) return err;

	int server_ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, server_ban_id);
	if(err) return err;

	auto args = req.get_args();
	if(args.find(std::string_view("expires")) != args.end()){
		std::string expires = std::string(req.get_arg("expires"));
		if(expires == "never")
			expires = "";
		try{
			tx.exec("UPDATE server_bans SET expiration_time = $1 WHERE user_id = $2", pqxx::params(expires.size() ? expires.c_str() : nullptr, server_ban_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Invalid date/time format", 400);
		}
	}
	tx.commit();

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

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_BAN_MEMBERS);
	if(err) return err;

	int server_ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, server_ban_id);
	if(err) return err;

	tx.exec("DELETE FROM server_bans WHERE user_id = $1 AND server_id = $2", pqxx::params(server_ban_id, server_id));
	tx.commit();
	return create_response::string(req, "Unbanned", 200);
}
