#include "resource/server_channels.h"
#include "resource/utils.h"

server_channels_resource::server_channels_resource(db_connection_pool& pool, auth_resource& auth): pool{pool}, auth{auth}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("PUT", true);
}

std::shared_ptr<http_response> server_channels_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, auth, tx, user_id, server_id);
	if(err) return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec_params("SELECT channel_id, name, type FROM channels WHERE server_id = $1", server_id);
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::channel_json_from_row(r[i]);

	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> server_channels_resource::render_PUT(const http_request& req)
{
	std::string name = std::string(req.get_header("name"));
	int type;
	auto err = resource_utils::parse_index(req, "type", type);
	if(err) return err;
	if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
		return std::shared_ptr<http_response>(new string_response("Unknown channel type", 400));

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	err = resource_utils::parse_server_id(req, auth, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	pqxx::result r = tx.exec_params("SELECT channel_id, name, type FROM channels WHERE server_id = $1", server_id);
	if(r.size() >= max_per_server)
		return std::shared_ptr<http_response>(new string_response("Server has more than " + std::to_string(max_per_server) + " channels", 403));

	int channel_id;
	try{
		r = tx.exec_params("INSERT INTO channels(server_id, name, type) VALUES($1, $2, $3) RETURNING channel_id", server_id, name, type);
		channel_id = r[0]["channel_id"].as<int>();
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Channel name is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response(std::to_string(channel_id), 200));
}
