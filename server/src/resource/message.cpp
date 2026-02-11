#include "resource/message.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

#include <iostream>

std::shared_ptr<http_response> message_get_params(const http_request& req, pqxx::work& tx, unsigned max_get_count,
							int& user_id, int& server_id, int& channel_id,
							int& start_id, int& count, std::string& id_query)
{
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	err = resource_utils::parse_index(req, "start_id", start_id, 0);
	if(err) return err;
	err = resource_utils::parse_index(req, "count", count, 0, max_get_count);
	if(err) return err;

	std::string order;
	err = resource_utils::parse_order(req, order);
	if(err) return err;

	id_query = "AND message_id " + std::string(order == "DESC" ? "<=" : ">=") + " $2 ORDER BY sent " + order + " LIMIT $3";

	return nullptr;
}

channel_messages_resource::channel_messages_resource(db_connection_pool& pool, socket_main_server& sserv) : base_resource(), pool{pool}, sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("POST", true);
}

std::shared_ptr<http_response> channel_messages_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id, server_id, channel_id;
	int start_id, count;
	std::string id_query;
	auto err = message_get_params(req, tx, max_get_count,
					user_id, server_id, channel_id,
					start_id, count, id_query);
	if(err)
		return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE channel_id = $1 " + id_query, pqxx::params(channel_id, start_id, count));
	for(size_t i = 0; i < r.size(); ++i){
		nlohmann::json mes = resource_utils::message_json_from_row(r[i]);
		if(i < r.size() - 1)
			mes["next_id"] = r[i + 1]["message_id"].as<int>();
		res += mes;
	}
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> channel_messages_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	std::string text = std::string(req.get_content());
	if(!text.size())
		return create_response::string(req, "Empty messages are forbidden", 400);

	int user_id, server_id, channel_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_CREATE_MESSAGES);
	if(err) return err;

	int message_id;
	socket_event ev;
	try{
		pqxx::result r = tx.exec("INSERT INTO messages(channel_id, server_id, author_id, sent, last_edited, text) VALUES($1, $2, $3, now(), now(), $4) RETURNING message_id, author_id, sent, last_edited, text, channel_id", pqxx::params(channel_id, server_id, user_id, text));
		message_id = r[0]["message_id"].as<int>();
		ev.data = resource_utils::message_json_from_row(r[0]);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Message is too long", 400);
	}
	tx.commit();

	resource_utils::json_set_ids(ev.data, server_id, channel_id);
	ev.name = "message_created";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, std::to_string(message_id), 200);
}


channel_messages_search_resource::channel_messages_search_resource(db_connection_pool& pool) : pool{pool}
{
	set_allowing("POST", true);
}
std::shared_ptr<http_response> channel_messages_search_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id, server_id, channel_id;
	int start_id, count;
	std::string id_query;
	auto err = message_get_params(req, tx, max_get_count,
					user_id, server_id, channel_id,
					start_id, count, id_query);
	if(err)
		return err;

	nlohmann::json body;
	err = resource_utils::json_from_content(req, body);
	if(err)
		return err;

	pqxx::params pr(channel_id, start_id, count);

	std::string q_where = "";
	

	if(body["content"].type() == nlohmann::json::value_t::string){
		pr.append(body["content"].get<std::string>());
		q_where += " AND text LIKE '%' || $" + std::to_string(pr.size()) + " || '%'";
	} else if(body["content"].is_array()){
		const auto& content = body["content"];
		if(content.size() > 16)
			return create_response::string(req, "content array is bigger than 16", 400);
		for(auto val = content.begin(); val != content.end(); ++val){
			if(val->type() != nlohmann::json::value_t::string)
				return create_response::string(req, "content array contains a non-string member", 400);
			pr.append(val->get<std::string>());
			q_where += " AND text LIKE '%' || $" + std::to_string(pr.size()) + " || '%'";
		}
	}

	if(body["author_id"].type() == nlohmann::json::value_t::number_unsigned){
		pr.append(body["author_id"].get<unsigned>());
		q_where += " AND author_id = $" + std::to_string(pr.size());
	}

	if(body["date_from"].type() == nlohmann::json::value_t::string){
		pr.append(body["date_from"].get<std::string>());
		q_where += " AND sent >= $" + std::to_string(pr.size());
	}
	if(body["date_until"].type() == nlohmann::json::value_t::string){
		pr.append(body["date_until"].get<std::string>());
		q_where += " AND sent <= $" + std::to_string(pr.size());
	}

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r;
	try{
		r = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE channel_id = $1 " + q_where + " " + id_query, pr);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid date/time format in search query", 400);
	}
	for(size_t i = 0; i < r.size(); ++i){
		nlohmann::json mes = resource_utils::message_json_from_row(r[i]);
		if(i < r.size() - 1)
			mes["next_id"] = r[i + 1]["message_id"].as<int>();
		res += mes;
	}
	return create_response::string(req, res.dump(), 200);
}


message_resource::message_resource(db_connection_pool& pool, socket_main_server& sserv) : pool{pool}, sserv{sserv}
{
	set_allowing("GET", true);
	set_allowing("PUT", true);
	set_allowing("DELETE", true);
}

std::shared_ptr<http_response> message_resource::render_GET(const http_request& req)
{
	base_resource::render_GET(req);

	int user_id, server_id, channel_id, message_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_message_id(req, tx, user_id, server_id, channel_id, message_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE message_id = $1", pqxx::params(message_id));
	nlohmann::json res = resource_utils::message_json_from_row(r[0]);
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> message_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	std::string text = std::string(req.get_content());
	if(!text.size())
		return create_response::string(req, "Empty messages are forbidden", 400);

	int user_id, server_id, channel_id, message_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_message_id(req, tx, user_id, server_id, channel_id, message_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT author_id FROM messages WHERE message_id = $1", pqxx::params(message_id));
	if(r[0]["author_id"].as<int>() != user_id)
		return create_response::string(req, "User is not the author of the mssage", 403);

	socket_event ev;
	try{
		r = tx.exec("UPDATE messages SET text = $1, last_edited = now() WHERE message_id = $2 RETURNING message_id, last_edited, text, channel_id", pqxx::params(text, message_id));
		ev.data = resource_utils::message_update_json_from_row(r[0]);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Message is too long", 400);
	}
	tx.commit();

	resource_utils::json_set_ids(ev.data, server_id, channel_id);
	ev.name = "message_edited";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, "Changed", 200);
}
std::shared_ptr<http_response> message_resource::render_DELETE(const http_request& req)
{
	base_resource::render_DELETE(req);

	int user_id, server_id, channel_id, message_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_message_id(req, tx, user_id, server_id, channel_id, message_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT author_id FROM messages WHERE message_id = $1", pqxx::params(message_id));
	if(r[0]["author_id"].as<int>() != user_id){ // not an author, but still can have the permission
		err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_DELETE_MESSAGES);
		if(err) return err;
	}

	tx.exec("DELETE FROM messages WHERE message_id = $1", pqxx::params(message_id));
	tx.commit();

	socket_event ev;
	ev.data["id"] = message_id;
	resource_utils::json_set_ids(ev.data, server_id, channel_id);
	ev.name = "message_deleted";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, "Deleted", 200);
}
