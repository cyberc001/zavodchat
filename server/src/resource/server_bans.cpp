#include "resource/server_bans.h"
#include "resource/utils.h"

server_bans_resource::server_bans_resource(db_connection_pool& pool) : pool{pool}
{
	ban_time_thr = std::thread(server_bans_resource::ban_time_func, std::ref(*this));

	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_bans_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int start;
	err = resource_utils::parse_index(req, "start", start, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, max_get_count);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec_params("SELECT user_id, name, avatar, status FROM server_bans NATURAL JOIN users WHERE server_id = $1 LIMIT $2 OFFSET $3", server_id, count, start);
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::user_json_from_row(r[i]);

	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> server_bans_resource::render_PUT(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int ban_user_id;
	err = resource_utils::parse_index(req, "ban_user_id", ban_user_id);
	if(err) return err;
	if(user_id == ban_user_id) // !!!also an established owner of the server
		return std::shared_ptr<http_response>(new string_response("Owner cannot ban themselves", 403));

	pqxx::result r = tx.exec_params("SELECT user_id FROM server_bans WHERE user_id = $1 AND server_id = $2", ban_user_id, server_id);
	if(r.size())
		return std::shared_ptr<http_response>(new string_response("Already banned", 202));

	std::string expires;
	err = resource_utils::parse_timestamp(req, "expires", expires);
	if(err) return err;

	try{
		tx.exec_params("INSERT INTO server_bans(user_id, server_id, expiration_time) VALUES ($1, $2, $3)", ban_user_id, server_id, expires.size() ? expires.c_str() : nullptr);
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Invalid date/time format", 400));
	}
	tx.exec_params("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2", ban_user_id, server_id);
	tx.commit();

	return std::shared_ptr<http_response>(new string_response("Banned", 200));
}

void server_bans_resource::ban_time_func(server_bans_resource& inst)
{
	for(;;){
		db_connection conn = inst.pool.hold();
		pqxx::work tx{*conn};
		tx.exec_params("DELETE FROM server_bans WHERE expiration_time IS NOT NULL AND expiration_time < now()");
		tx.commit();
		std::this_thread::sleep_for(std::chrono::seconds(inst.cleanup_period));
	}
}


server_ban_id_resource::server_ban_id_resource(db_connection_pool& pool) : pool{pool}
{
	disallow_all();
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_ban_id_resource::render_POST(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int server_ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, server_ban_id);
	if(err) return err;

	auto hdrs = req.get_headers();
	if(hdrs.find("expires") != hdrs.end()){
		std::string expires = std::string(req.get_header("expires"));
		if(expires == "never")
			expires = "";
		try{
			tx.exec_params("UPDATE server_bans SET expiration_time = $1 WHERE user_id = $2", expires.size() ? expires.c_str() : nullptr, server_ban_id);
		} catch(pqxx::data_exception& e){
			return std::shared_ptr<http_response>(new string_response("Invalid date/time format", 400));
		}
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response("Unbanned", 200));
}

std::shared_ptr<http_response> server_ban_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int server_ban_id;
	err = resource_utils::parse_server_ban_id(req, server_id, tx, server_ban_id);
	if(err) return err;

	tx.exec_params("DELETE FROM server_bans WHERE user_id = $1 AND server_id = $2", server_ban_id, server_id);
	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Unbanned", 200));
}
