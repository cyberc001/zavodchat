#include "resource/server_channels.h"
#include "resource/utils.h"

server_channel_resource::server_channel_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
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

	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> server_channel_resource::render_PUT(const http_request& req)
{
	std::string name = std::string(req.get_arg("name"));
	int type;
	auto err = resource_utils::parse_index(req, "type", type);
	if(err) return err;
	if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
		return std::shared_ptr<http_response>(new string_response("Unknown channel type", 400));

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT channel_id, name, type FROM channels WHERE server_id = $1", pqxx::params(server_id));
	if(r.size() >= max_per_server)
		return std::shared_ptr<http_response>(new string_response("Server has more than " + std::to_string(max_per_server) + " channels", 403));

	int channel_id;
	try{
		r = tx.exec("INSERT INTO channels(server_id, name, type) VALUES($1, $2, $3) RETURNING channel_id", pqxx::params(server_id, name, type));
		channel_id = r[0]["channel_id"].as<int>();
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Channel name is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response(std::to_string(channel_id), 200));
}

server_channel_id_resource::server_channel_id_resource(db_connection_pool& pool): pool{pool}
{
	disallow_all();
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
	return std::shared_ptr<http_response>(new string_response(resource_utils::channel_json_from_row(r[0]).dump(), 200));
}
std::shared_ptr<http_response> server_channel_id_resource::render_POST(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	auto args = req.get_args();
	if(args.find(std::string_view("name")) != args.end()){
		std::string name = std::string(req.get_arg("name"));
		try{
			tx.exec("UPDATE channels SET name = $1 WHERE channel_id = $2", pqxx::params(name, channel_id));
		} catch(pqxx::data_exception& e){
			return std::shared_ptr<http_response>(new string_response("Channel name is too long", 400));
		}
	}
	if(args.find(std::string_view("type")) != args.end()){
		int type;
		auto err = resource_utils::parse_index(req, "type", type);
		if(err) return err;
		if(type != CHANNEL_TEXT && type != CHANNEL_VOICE)
			return std::shared_ptr<http_response>(new string_response("Unknown channel type", 400));
		tx.exec("UPDATE channels SET type = $1 WHERE channel_id = $2", pqxx::params(type, channel_id));
	}
	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Changed", 200));
}
std::shared_ptr<http_response> server_channel_id_resource::render_DELETE(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, tx, user_id, server_id);
	if(err) return err;

	err = resource_utils::check_server_owner(user_id, server_id, tx);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	tx.exec("DELETE FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	tx.commit();
	return std::shared_ptr<http_response>(new string_response("Deleted", 200));
}
