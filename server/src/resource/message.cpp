#include "resource/message.h"
#include "resource/utils.h"
#include "resource/role_utils.h"

channel_messages_resource::channel_messages_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv):
	base_resource(ws, "/channels/{channel_id}/messages", pool, cfg),
	sserv{sserv}
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
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	pqxx::params pr(channel_id);
	std::string pg_query;
	err = resource_utils::pagination_query(req, cfg, "message_id", pr, pg_query);
	if(err)
		return err;

	nlohmann::json res = nlohmann::json::array();
	pqxx::result r = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE channel_id = $1 " + pg_query, pr);
	for(size_t i = 0; i < r.size(); ++i){
		pqxx::result r_att = tx.exec("SELECT type, content FROM message_attachments WHERE message_id = $1", pqxx::params(r[i]["message_id"].as<int>()));
		res += resource_utils::message_json_from_row(r[i], r_att);
	}
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> channel_messages_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	int user_id, server_id, channel_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	if(server_id != -1){
		err = role_utils::check_permission1(req, tx, server_id, user_id, PERM1_CREATE_MESSAGES);
		if(err) return err;
	}

	nlohmann::json body;
	err = resource_utils::json_from_content(req, body);
	if(err)
		return err;

	if(body["text"].type() != nlohmann::json::value_t::string)
		return create_response::string(req, "Message text is not a string", 400);
	std::string text = body["text"].get<std::string>();
	if(!text.size() && (!body["attachments"].is_array() || !body["attachments"].size()))
		return create_response::string(req, "Empty messages without attachments are forbidden", 400);

	int message_id;
	pqxx::result msg_res;
	try{
		msg_res = tx.exec("INSERT INTO messages(channel_id, author_id, sent, last_edited, text) VALUES($1, $2, now(), now(), $3) RETURNING message_id, author_id, sent, last_edited, text, channel_id", pqxx::params(channel_id, user_id, text));
		message_id = msg_res[0]["message_id"].as<int>();
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Message is too long", 400);
	}

	std::vector<pqxx::row> attachment_rows;
	if(body["attachments"].is_array()){
		auto& att = body["attachments"];
		if(att.size() > cfg.max_attachments)
			return create_response::string(req, "Too many attachments", 400);
		for(auto val = att.begin(); val != att.end(); ++val){
			std::string content; unsigned att_type;
			err = parse_attachment(req, *val, content, att_type);
			if(err) return err;
			pqxx::params pr(message_id, att_type, content);

			try {
				pqxx::result r = tx.exec("INSERT INTO message_attachments(message_id, type, content) VALUES($1, $2, $3) RETURNING message_id, type, content", pr);
				attachment_rows.push_back(r[0]);
			} catch(pqxx::data_exception& e){
				return create_response::string(req, "Attachment content is too long", 400);
			}
		}
	}

	tx.commit();

	socket_event ev;
	ev.data = resource_utils::message_json_from_row(msg_res[0], attachment_rows);
	resource_utils::json_set_ids(ev.data, server_id, channel_id);
	ev.name = "message_created";
	sserv.send_to_channel(channel_id, tx, ev);

	return create_response::string(req, std::to_string(message_id), 200);
}

bool channel_messages_resource::is_valid_attachment_type(unsigned type)
{
	return type <= MESSAGE_ATTACHMENT_IMAGE;
}
std::shared_ptr<http_response> channel_messages_resource::parse_attachment(const http_request& req, nlohmann::json& val,
										std::string& content, unsigned& att_type)
{
	if(!val.is_object())
		return create_response::string(req, "An attachment is not an object", 400);
	if(val["type"].type() != nlohmann::json::value_t::number_unsigned)
		return create_response::string(req, "An attachment type is not an unsigned integer", 400);
	if(val["content"].type() != nlohmann::json::value_t::string)
		return create_response::string(req, "An attachment content is not string", 400);

	content = val["content"].get<std::string>();
	att_type = val["type"].get<unsigned>();
	if(!is_valid_attachment_type(att_type))
		return create_response::string(req, "Invalid attachment type " + std::to_string(att_type), 400);
	return nullptr;
}

channel_messages_search_resource::channel_messages_search_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/channels/{channel_id}/messages_search", pool, cfg),
	sserv{sserv}
{
	set_allowing("POST", true);
}
std::shared_ptr<http_response> channel_messages_search_resource::render_POST(const http_request& req)
{
	base_resource::render_POST(req);

	db_connection conn = pool.hold();
	pqxx::work tx{*conn};

	int user_id, server_id, channel_id;
	auto err = resource_utils::parse_channel_id(req, tx, user_id, server_id, channel_id);
	if(err) return err;

	std::string pg_query;
	pqxx::params pr(channel_id);
	err = resource_utils::pagination_query(req, cfg, "message_id", pr, pg_query);
	if(err)
		return err;

	nlohmann::json body;
	err = resource_utils::json_from_content(req, body);
	if(err)
		return err;

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
		r = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE channel_id = $1 " + q_where + " " + pg_query, pr);
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid date/time format in search query", 400);
	}
	for(size_t i = 0; i < r.size(); ++i){
		pqxx::result r_att = tx.exec("SELECT type, content FROM message_attachments WHERE message_id = $1", pqxx::params(r[i]["message_id"].as<int>()));
		res += resource_utils::message_json_from_row(r[i], r_att);
	}
	return create_response::string(req, res.dump(), 200);
}


message_resource::message_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
						socket_main_server& sserv):
	base_resource(ws, "/messages/{message_id}", pool, cfg),
	sserv{sserv}
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
	pqxx::result r_att = tx.exec("SELECT type, content FROM message_attachments WHERE message_id = $1", pqxx::params(r[0]["message_id"].as<int>()));
	nlohmann::json res = resource_utils::message_json_from_row(r[0], r_att);
	return create_response::string(req, res.dump(), 200);
}
std::shared_ptr<http_response> message_resource::render_PUT(const http_request& req)
{
	base_resource::render_PUT(req);

	int user_id, server_id, channel_id, message_id;
	db_connection conn = pool.hold();
	pqxx::work tx{*conn};
	auto err = resource_utils::parse_message_id(req, tx, user_id, server_id, channel_id, message_id);
	if(err) return err;

	pqxx::result r = tx.exec("SELECT author_id FROM messages WHERE message_id = $1", pqxx::params(message_id));
	if(r[0]["author_id"].as<int>() != user_id)
		return create_response::string(req, "User is not the author of the mssage", 403);


	nlohmann::json body;
	err = resource_utils::json_from_content(req, body);
	if(err)
		return err;

	bool edited = false;

	// Change text if requested, get message row regardless
	pqxx::result msg_res;
	if(body["text"].type() == nlohmann::json::value_t::string){
		std::string text = body["text"].get<std::string>();
		if(!text.size()){
			pqxx::result r = tx.exec("SELECT type, content FROM message_attachments WHERE message_id = $1", pqxx::params(message_id));
			if(!r.size())
				return create_response::string(req, "Empty messages without attachments are forbidden", 400);
		}
		msg_res = tx.exec("UPDATE messages SET text = $1 WHERE message_id = $2 RETURNING message_id, author_id, sent, last_edited, text, channel_id", pqxx::params(text, message_id));
		edited = true;
	} else
		msg_res = tx.exec("SELECT message_id, author_id, sent, last_edited, text FROM messages WHERE message_id = $1", pqxx::params(message_id));

	// Replace the whole attachments array if requested, get attachment rows regardless
	std::vector<pqxx::row> attachment_rows;
	if(body["attachments"].is_array()){
		auto& att = body["attachments"];
		if(att.size() > cfg.max_attachments)
			return create_response::string(req, "Too many attachments", 400);

		tx.exec("DELETE FROM message_attachments WHERE message_id = $1", pqxx::params(message_id));
		for(auto val = att.begin(); val != att.end(); ++val){
			std::string content; unsigned att_type;
			err = channel_messages_resource::parse_attachment(req, *val, content, att_type);
			if(err) return err;
			pqxx::params pr(message_id, att_type, content);

			try {
				pqxx::result r = tx.exec("INSERT INTO message_attachments(message_id, type, content) VALUES($1, $2, $3) RETURNING message_id, type, content", pr);
				attachment_rows.push_back(r[0]);
			} catch(pqxx::data_exception& e){
				return create_response::string(req, "Attachment content is too long", 400);
			}
		}

		edited = true;
	} else {
		pqxx::result att_res = tx.exec("SELECT type, content FROM message_attachments WHERE message_id = $1", pqxx::params(message_id));
		for(auto i = att_res.begin(); i != att_res.end(); ++i)
			attachment_rows.push_back(*i);
	}

	if(edited){
		pqxx::result r = tx.exec("UPDATE messages SET last_edited = now() WHERE message_id = $1 RETURNING last_edited", pqxx::params(message_id));
		tx.commit();

		socket_event ev;
		msg_res[0]["last_edited"] = r[0]["last_edited"];
		ev.data = resource_utils::message_json_from_row(msg_res[0], attachment_rows);
		resource_utils::json_set_ids(ev.data, server_id, channel_id);
		ev.name = "message_edited";
		sserv.send_to_channel(channel_id, tx, ev);
	}

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
