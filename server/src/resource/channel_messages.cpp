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
