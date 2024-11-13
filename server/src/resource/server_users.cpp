#include "resource/server_users.h"
#include "resource/utils.h"

#include <iostream>

server_users_resource::server_users_resource(db_connection_pool& pool) : pool{pool}
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> server_users_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int start;
	err = resource_utils::parse_index(req, "start", start, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, max_get_count);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec_params("SELECT user_id, name, avatar, status FROM user_x_server NATURAL JOIN users WHERE server_id = $1 LIMIT $2 OFFSET $3", server_id, count, start);
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::user_json_from_row(r[i]);

	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
