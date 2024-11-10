#include "resource/channel_messages.h"
#include "resource/utils.h"

#include <iostream>

channel_messages_resource::channel_messages_resource(db_connection_pool& pool, auth_resource& auth) : pool{pool}, auth{auth}
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> channel_messages_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, auth, tx, user_id, server_id);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	int start;
	err = resource_utils::parse_index(req, "start", start, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, max_get_count);
	if(err) return err;

	std::string order = "DESC";
	auto hdrs = req.get_headers();
	if(hdrs.find("order") != hdrs.end()){
		int int_order;
		auto err = resource_utils::parse_index(req, "order", int_order);
		if(err) return err;
		if(int_order != ORDER_ASC && int_order != ORDER_DESC)
			return std::shared_ptr<http_response>(new string_response("Unknown order type", 400));
		if(int_order == ORDER_ASC)
			order = "ASC";
	}

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec_params("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE channel_id = $1 ORDER BY sent " + order + " LIMIT $2 OFFSET $3", channel_id, count, start);
	for(size_t i = 0; i < r.size(); ++i)
		res += resource_utils::message_json_from_row(r[i]);
	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}


channel_message_id_resource::channel_message_id_resource(db_connection_pool& pool, auth_resource& auth) : pool{pool}, auth{auth}
{
	disallow_all();
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> channel_message_id_resource::render_GET(const http_request& req)
{
	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, auth, tx, user_id, server_id);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	int message_id;
	err = resource_utils::parse_message_id(req, channel_id, tx, message_id);
	if(err) return err;

	pqxx::result r = tx.exec_params("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE message_id = $1", message_id);
	nlohmann::json res = resource_utils::message_json_from_row(r[0]);
	return std::shared_ptr<http_response>(new string_response(res.dump(), 200));
}
std::shared_ptr<http_response> channel_message_id_resource::render_POST(const http_request& req)
{
	std::string text = std::string(req.get_header("text"));

	int user_id, server_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_server_id(req, auth, tx, user_id, server_id);
	if(err) return err;

	int channel_id;
	err = resource_utils::parse_channel_id(req, server_id, tx, channel_id);
	if(err) return err;

	int message_id;
	err = resource_utils::parse_message_id(req, channel_id, tx, message_id);
	if(err) return err;

	pqxx::result r = tx.exec_params("SELECT author_id FROM messages WHERE message_id = $1", message_id);
	if(r[0]["author_id"].as<int>() != user_id)
		return std::shared_ptr<http_response>(new string_response("User is not the author of the mssage", 403));

	try{
		tx.exec_params("UPDATE messages SET text = $1, last_edited = now() WHERE message_id = $2", text, message_id);
	} catch(pqxx::data_exception& e){
		return std::shared_ptr<http_response>(new string_response("Message is too long", 400));
	}
	tx.commit();

	return std::shared_ptr<http_response>(new string_response("Changed", 200));
}
