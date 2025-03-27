#include "resource/server_invites.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

server_invites_resource::server_invites_resource(db_connection_pool& pool, socket_main_server& sserv): pool{pool}, sserv{sserv}
{
	invite_time_thr = std::thread(server_invites_resource::invite_time_func, std::ref(*this));

	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> server_invites_resource::render_GET(const http_request& req)
{
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id;
	auto err = resource_utils::parse_session_token(req, tx, user_id);
	if(err) return err;

	std::string invite_id;
	err = resource_utils::parse_invite_id(req, tx, invite_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT invite_id, server_id, expiration_time FROM server_invites WHERE invite_id = $1 AND expiration_time IS NULL OR expiration_time > now()", pqxx::params(invite_id));
	if(!r.size())
		return create_response::string("Invite has expired", 403);
	int server_id = r[0]["server_id"].as<int>();

	r = tx.exec("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(user_id, server_id));
	if(r.size())
		return create_response::string("Already joined", 202);
	r = tx.exec("SELECT user_id FROM server_bans WHERE user_id = $1 AND server_id = $2", pqxx::params(user_id, server_id));
	if(r.size())
		return create_response::string("User is banned", 403);

	tx.exec("INSERT INTO user_x_server(user_id, server_id, role_id) VALUES($1, $2, $3)", pqxx::params(user_id, server_id, role_utils::find_default_role(tx, server_id)));
	tx.commit();

	socket_event ev;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.data["id"] = user_id;
	ev.name = "user_joined";
	sserv.send_to_server(server_id, tx, ev);
	
	return create_response::string("Joined", 200);
}

void server_invites_resource::invite_time_func(server_invites_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec("DELETE FROM server_invites WHERE expiration_time IS NOT NULL AND expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.cleanup_period));
	}
}


server_id_invites_resource::server_id_invites_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_id_invites_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;
	
	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT invite_id, server_id, expiration_time FROM server_invites WHERE server_id = $1", pqxx::params(server_id));

	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::invite_json_from_row(r[i]);

	return create_response::string(res.dump(), 200);
}
std::shared_ptr<http_response> server_id_invites_resource::render_PUT(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;
	
	pqxx::result r = tx.exec("SELECT server_id FROM server_invites WHERE server_id = $1", pqxx::params(server_id));
	if(r.size() >= max_per_server)
		return create_response::string("Server has more than " + std::to_string(max_per_server) + " invites", 403);

	std::string expires;
	err = resource_utils::parse_timestamp(req, "expires", expires);
	if(err) return err;
	try{
		r = tx.exec("INSERT INTO server_invites(invite_id, server_id, expiration_time) VALUES(gen_random_uuid(), $1, $2) RETURNING invite_id", pqxx::params(server_id, expires.size() ? expires.c_str() : nullptr));
	} catch(pqxx::data_exception& e){
		return create_response::string("Invalid date/time format", 400);
	}
	tx.commit();

	return create_response::string(r[0]["invite_id"].as<std::string>(), 200);
}


server_invite_id_resource::server_invite_id_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_invite_id_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	std::string invite_id;
	err = resource_utils::parse_invite_id(req, server_id, tx, invite_id);
	if(err) return err;
	
	pqxx::result r = tx.exec("SELECT invite_id, server_id, expiration_time FROM server_invites WHERE invite_id = $1", pqxx::params(invite_id));
	return create_response::string(resource_utils::invite_json_from_row(r[0]).dump(), 200);
}
std::shared_ptr<http_response> server_invite_id_resource::render_POST(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	std::string invite_id;
	err = resource_utils::parse_invite_id(req, server_id, tx, invite_id);
	if(err) return err;

	auto args = req.get_args();
	if(args.find(std::string_view("expires")) != args.end()){
		std::string expires = std::string(req.get_arg("expires"));
		if(expires == "never")
			expires = "";
		try{
			tx.exec("UPDATE server_invites SET expiration_time = $1 WHERE invite_id = $2", pqxx::params(expires.size() ? expires.c_str() : nullptr, invite_id));
		} catch(pqxx::data_exception& e){
			return create_response::string("Invalid date/time format", 400);
		}
	}
	tx.commit();

	return create_response::string("Changed", 200);
}
std::shared_ptr<http_response> server_invite_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	std::string invite_id;
	err = resource_utils::parse_invite_id(req, server_id, tx, invite_id);
	if(err) return err;

	tx.exec("DELETE FROM server_invites WHERE invite_id = $1", pqxx::params(invite_id));
	tx.commit();
	return create_response::string("Deleted", 200);
}
