#include "resource/server_users.h"
#include "resource/utils.h"

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
	pqxx::result r = tx.exec("SELECT user_id, name, avatar, status FROM user_x_server NATURAL JOIN users WHERE server_id = $1 LIMIT $2 OFFSET $3", pqxx::params(server_id, count, start));
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::user_json_from_row(r[i]);

	return create_response::string(res.dump(), 200);
}

server_user_id_resource::server_user_id_resource(db_connection_pool& pool, socket_main_server& sserv) : pool{pool}, sserv{sserv}
{
	disallow_all();
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> server_user_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int server_user_id;
	err = resource_utils::parse_server_user_id(req, server_id, tx, server_user_id);
	if(err) return err;

	if(user_id == server_user_id) // !!!also an established owner of the server
		return create_response::string("Owner cannot kick themselves", 403);

	socket_event ev;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.data["id"] = user_id;
	ev.name = "user_left";
	sserv.send_to_server(server_id, tx, ev);

	tx.exec("DELETE FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	tx.commit();

	return create_response::string("Kicked", 200);
}
