#include "resource/channel.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

server_channel_resource::server_channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv, socket_vc_server& vcserv):
	base_resource(ws, "/servers/{server_id}/channels", pool, cfg),
	sserv{sserv}, vcserv{vcserv}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> server_channel_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE server_id = $1", pqxx::params(server_id));

	for(size_t i = 0; i < r.size(); ++i){
		nlohmann::json channel_json = resource_utils::channel_json_from_row(r[i]);
		if(channel_json["type"] == CHANNEL_VOICE)
			channel_json["vc_users"] = vcserv.get_channel_users(channel_json["id"]);
		res += channel_json;
	}

	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> server_channel_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

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
	if(r.size() >= cfg.max_channels_per_server)
		return create_response::string(req, "Server has more than " + std::to_string(cfg.max_channels_per_server) + " channels", 403);

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

channel_resource::channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv, socket_vc_server& vcserv):
	base_resource(ws, "/channels/{channel_id}", pool, cfg),
	sserv{sserv}, vcserv{vcserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}
std::shared_ptr<http_response> channel_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT channel_id, name, type, user1_id, user2_id FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	nlohmann::json channel_json = resource_utils::channel_json_from_row(r[0], user_id);

	if(channel_json["type"] == CHANNEL_VOICE)
		channel_json["vc_users"] = vcserv.get_channel_users(channel_id);

	return create_response::string(req, channel_json.dump(), 200);
}
std::shared_ptr<http_response> channel_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot change a DM channel", 403);

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_CHANNELS);
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
		err = resource_utils::parse_index(req, "type", type);
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
std::shared_ptr<http_response> channel_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id == -1)
		return create_response::string(req, "Cannot change a DM channel", 403);

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_CHANNELS);
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


channel_user_id_resource::channel_user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_vc_server& vcserv):
	base_resource(ws, "/channels/{channel_id}/users/{user_id}", pool, cfg),
	vcserv{vcserv}
{
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> channel_user_id_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, channel_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	int other_user_id;
	err = resource_utils::parse_user_id(req, tx, other_user_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT type FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	if(r[0]["type"].as<int>() != CHANNEL_VOICE)
		return create_response::string(req, "Not a voice channel", 400);

	if(server_id > -1){
		err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_MANAGE_VC);
		if(err) return err;
	} else {
		// If user themselves are calling/in the call, forbid kicking the other participant
		if(vcserv.has_user(channel_id, user_id))
			return create_response::string(req, "Cannot kick the other user while in the call", 403);
	}

	if(vcserv.kick_user(channel_id, other_user_id, server_id > -1 ? "Kicked by administrator" : "Call denied"))
		return create_response::string(req, "Kicked", 200);
	return create_response::string(req, "User is not in this voice channel", 404);
}
