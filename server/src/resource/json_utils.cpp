#include "resource/json_utils.h"
#include "resource/notification_utils.h"

std::shared_ptr<http_response> json_utils::from_content(const http_request& req, nlohmann::json& data)
{
	try {
		data = nlohmann::json::parse(req.get_content());
	} catch(nlohmann::json::parse_error& e){
		return create_response::string(req, "Invalid JSON content", 400);
	}
	return nullptr;
}

void json_utils::set_ids(nlohmann::json& data, int server_id, int channel_id)
{
	if(server_id != -1)
		data["server_id"] = server_id;
	if(channel_id != -1)
		data["channel_id"] = channel_id;
}

nlohmann::json json_utils::user_from_row(const pqxx::row& r)
{
	nlohmann::json res = {
		{"id", r["user_id"].as<int>()},
		{"name", r["name"].as<std::string>()},
		{"status", r["status"].as<int>()}
	};
	if(!r["avatar"].is_null())
		res += {"avatar", r["avatar"].as<std::string>()};
	return res;
}

nlohmann::json json_utils::server_from_row(const pqxx::row& r, bool notifications)
{
	nlohmann::json res = {{"id", r["server_id"].as<int>()},
				{"name", r["name"].as<std::string>()}};
	if(!r["avatar"].is_null())
		res += {"avatar", r["avatar"].as<std::string>()};
	if(notifications && !r["notification_count"].is_null())
		res += {"notifications", r["notification_count"].as<int>()};
	return res;
}

nlohmann::json json_utils::channel_from_row(const pqxx::row& r, const pqxx::result* role_rows,
						bool has_notifications, int user_id)
{
	nlohmann::json res = {
		{"id", r["channel_id"].as<int>()},
		{"type", r["type"].as<int>()},
		{"wl_users", nlohmann::json::array()},
		{"wl_roles", nlohmann::json::array()}
	};
	if(!r["name"].is_null())
		res += {"name", r["name"].as<std::string>()};

	if(role_rows && role_rows->size()){
		res["roles"] = nlohmann::json::array();
		for(size_t i = 0; i < role_rows->size(); ++i)
			res["roles"] += {{"id", (*role_rows)[i]["role_id"].as<int>()},
					{"perms1", (*role_rows)[i]["perms1"].as<int>()}};
	}

	if(has_notifications && !r["notification_count"].is_null())
		res += {"notifications", r["notification_count"].as<int>()};
	if(user_id > -1 && !r["user1_id"].is_null()){
		int user1_id = r["user1_id"].as<int>(), user2_id = r["user2_id"].as<int>();
		res += {"other_user_id", user1_id == user_id ? user2_id : user1_id};
	}

	// Parse whitelists
	std::vector<int> wl_users = resource_utils::parse_psql_int_array(r["wl_users"]);
	if(wl_users.size())
		for(size_t i = 0; i < wl_users.size(); ++i)
			res["wl_users"].push_back(wl_users[i]);

	std::vector<int> wl_roles = resource_utils::parse_psql_int_array(r["wl_roles"]);
	if(wl_roles.size())
		for(size_t i = 0; i < wl_roles.size(); ++i)
			res["wl_roles"].push_back(wl_roles[i]);

	return res;
}

nlohmann::json json_utils::message_from_row(const pqxx::row& msg_row, const std::vector<pqxx::row>& attachment_rows)
{
	nlohmann::json r = {
		{"id", msg_row["message_id"].as<int>()},
		{"author_id", msg_row["author_id"].as<int>()},
		{"sent", msg_row["sent"].as<std::string>()},
		{"edited", msg_row["last_edited"].as<std::string>()},
		{"text", msg_row["text"].as<std::string>()},
		{"attachments", nlohmann::json::array()},
		{"mentions", nlohmann::json::array()}
	};
	for(auto i = attachment_rows.begin(); i != attachment_rows.end(); ++i)
		r["attachments"].push_back({
			{"type", (*i)["type"].as<unsigned>()},
			{"content", (*i)["content"].as<std::string>()}
		});

	std::vector<int> mentions = resource_utils::parse_psql_int_array(msg_row["mentions"]);
	for(size_t i = 0; i < mentions.size(); i += 4){
		nlohmann::json m = {
			{"type", static_cast<mention_types>(mentions[i])},
			{"begin_i", mentions[i + 1]},
			{"end_i", mentions[i + 2]}
		};
		if(mentions[i + 3] > -1)
			m["id"] = mentions[i + 3];
		r["mentions"].push_back(m);	
	}
	return r;
}
nlohmann::json json_utils::message_from_row(const pqxx::row& msg_row, const pqxx::result& _attachment_rows)
{
	std::vector<pqxx::row> attachment_rows;
	for(auto i = _attachment_rows.begin(); i != _attachment_rows.end(); ++i)
		attachment_rows.push_back(*i);
	return message_from_row(msg_row, attachment_rows);
}

nlohmann::json json_utils::invite_from_row(const pqxx::row& r)
{
	return {
		{"id", r["invite_id"].as<std::string>()},
		{"server_id", r["server_id"].as<int>()},
		{"expires", r["expiration_time"].is_null() ? "never" : r["expiration_time"].as<std::string>()}
	};
}
nlohmann::json json_utils::ban_from_row(const pqxx::row& r)
{
	return {
		{"id", r["ban_id"].as<int>()},
		{"user", json_utils::user_from_row(r)},
		{"expires", r["expiration_time"].is_null() ? "never" : r["expiration_time"].as<std::string>()}
	};
}
