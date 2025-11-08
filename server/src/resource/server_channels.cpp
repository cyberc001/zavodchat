#include "resource/server_channels.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

server_channel_resource::server_channel_resource(db_connection_pool& pool, socket_main_server& sserv): base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_channel_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE server_id = $1", pqxx::params(server_id));

	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::channel_json_from_row(r[i]);

	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_channel_resource::render_PUT(const http_request& req)
{
	std::string name = std::string(req.get_arg("name"));
	int type;
	auto err = resource_utils::parse_index(req, "type", type);
	if(err) return err;
	if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
		return create_response::string(req, "Unknown channel type", 400);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE server_id = $1", pqxx::params(server_id));
	if(r.size() >= max_per_server)
		return create_response::string(req, "Server has more than " + std::to_string(max_per_server) + " channels", 403);

	int channel_id;
	socket_event ev;
	try{
		r = tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, $2, $3) RETURNING channel_id, name, type", pqxx::params(server_id, name, type));
		channel_id = r[0]["channel_id"].as<int>();
		ev.data = resource_utils::channel_json_from_row(r[0]);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Channel name is too long", 400);
	}
	tx.commit();

	resource_utils::json_set_ids(ev.data, server_id);
	ev.name = "channel_created";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, std::to_string(channel_id), 200);
}

server_channel_id_resource::server_channel_id_resource(db_connection_pool& pool, socket_main_server& sserv, socket_vc_server& vcserv): base_resource(), pool{pool}, sserv{sserv}, vcserv{vcserv}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
	set_allowing("DELETE", true);
}
std::shared_ptr<http_response> server_channel_id_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;
	pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	nlohmann::json channel_json = resource_utils::channel_json_from_row(r[0]);

	if(channel_json["type"] == CHANNEL_VOICE){ // send users currently speaking in channel
		std::vector<int> users;
		vcserv.get_channel_users(channel_id, users);
		channel_json["vc_users"] = nlohmann::json::array();
		for(auto it = users.begin(); it != users.end(); ++it)
			channel_json["vc_users"] += *it;
	}

	return create_response::string(req, channel_json.dump(), 200);
}
std::shared_ptr<http_response> server_channel_id_resource::render_POST(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	bool updated = false;
	auto args = req.get_args();
	if(args.find(std::string_view("name")) != args.end()){
		std::string name = std::string(req.get_arg("name"));
		try{
			tx.exec("UPDATE channels SET name = $1 WHERE channel_id = $2", pqxx::params(name, channel_id));
		} catch(pqxx::data_exception& e){
			return create_response::string(req, "Channel name is too long", 400);
		}
		updated = true;
	}
	if(args.find(std::string_view("type")) != args.end()){
		int type;
		if(err) return err;
		if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
			return create_response::string(req, "Unknown channel type", 400);
		tx.exec("UPDATE channels SET type = $1 WHERE channel_id = $2", pqxx::params(type, channel_id));
		updated = true;
	}
	tx.commit();

	if(updated){
		socket_event ev;
		pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
		ev.data = resource_utils::channel_json_from_row(r[0]);
		resource_utils::json_set_ids(ev.data, server_id);
		ev.name = "channel_edited";
		sserv.send_to_server(server_id, tx, ev);
	}

	return create_response::string(req, "Changed", 200);
}
std::shared_ptr<http_response> server_channel_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_CHANNELS);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	tx.exec("DELETE FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	tx.commit();

	socket_event ev;
	ev.data["id"] = channel_id;
	resource_utils::json_set_ids(ev.data, server_id);
	ev.name = "channel_deleted";
	sserv.send_to_server(server_id, tx, ev);

	return create_response::string(req, "Deleted", 200);
}
