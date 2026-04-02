#include "resource/utils.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iosfwd>

#include <iostream>

std::vector<std::string> create_response::origins;
void create_response::set_origins(std::vector<std::string> _origins)
{
	origins = _origins;
}

void create_response::add_cors(const http_request& req, http_response& res)
{
	res.with_header("Accept", "*/*");
	std::string og = std::string(req.get_header("origin"));
	if(std::find(origins.begin(), origins.end(), og) != origins.end())
		res.with_header("Access-Control-Allow-Origin", og);
	res.with_header("Access-Control-Allow-Credentials", "true");
	res.with_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
	res.with_header("Access-Control-Allow-Headers", "Origin, Content-Type, X-Auth-Token, X-Requested-With, Accept, Username, Password, DisplayName");
}

std::shared_ptr<http_response> create_response::string(const http_request& req, std::string str, int code)
{
	string_response* res = new string_response(str, code);
	add_cors(req, *res);
	return std::shared_ptr<http_response>(res);
}
std::shared_ptr<http_response> create_response::file(const http_request& req, std::string fpath)
{
	file_response* res = new file_response(fpath);

	// Copied checks from file_response.cpp for performance
	struct stat sb;
	if(stat(fpath.c_str(), &sb) == 0){
	        if(!S_ISREG(sb.st_mode))
			return create_response::string(req, "File does not exist", 404);
	} else
		return create_response::string(req, "File does not exist", 404);
	int fd = open(fpath.c_str(), O_RDONLY);
	if(fd == -1) return create_response::string(req, "File does not exist", 404);

	off_t size = lseek(fd, 0, SEEK_END);
	if(size == (off_t) -1) return create_response::string(req, "File does not exist", 404);
	close(fd);
	// End of copied checks

	add_cors(req, *res);
	return std::shared_ptr<http_response>(res);
}


time_t resource_utils::time_now()
{
	auto now = std::chrono::system_clock::now();
	return std::chrono::system_clock::to_time_t(now);
}

/* Parsing */

std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string arg_name, int& index)
{
	try{
		index = std::stoi(std::string(req.get_arg(arg_name)));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Couldn't parse '" + arg_name + "', got: " + std::string(req.get_arg(arg_name)), 400);
	}

	return nullptr;
}
std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string arg_name, int& index, int lower_bound)
{
	auto err = parse_index(req, arg_name, index);
	if(err) return err;

	if(index < lower_bound)
		return create_response::string(req, arg_name + " is invalid: " + std::to_string(index) + " is below " + std::to_string(lower_bound), 403);

	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string arg_name, int& index, int lower_bound, int upper_bound)
{
	auto err = parse_index(req, arg_name, index);
	if(err) return err;

	if(index < lower_bound)
		return create_response::string(req, arg_name + " is invalid: " + std::to_string(index) + " is below " + std::to_string(lower_bound), 403);
	if(index > upper_bound)
		return create_response::string(req, arg_name + " is invalid: " + std::to_string(index) + " is above " + std::to_string(upper_bound), 403);

	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_session_token(const http_request& req, pqxx::work& tx, int& user_id)
{
	pqxx::result r;
	try{
		r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(std::string(req.get_cookie("zavodchat_token"))));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid token", 400);
	}
	if(!r.size())
		return create_response::string(req, "Expired or invalid token", 401);
	user_id = r[0]["user_id"].as<int>();
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_timestamp(const http_request& req, std::string arg_name, std::string& ts)
{
	ts = std::string(req.get_arg(arg_name));
	if(!ts.size())
		return create_response::string(req, "Empty '" + arg_name + "'", 400);
	if(ts == "never")
		ts = "";
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_order(const http_request& req, std::string& order)
{
	order = "DESC";
	auto args = req.get_args();
	if(args.find(std::string_view("order")) != args.end()){
		int int_order;
		auto err = resource_utils::parse_index(req, "order", int_order);
		if(err) return err;
		if(int_order != ORDER_ASC && int_order != ORDER_DESC)
			return create_response::string(req, "Unknown order type", 400);
		if(int_order == ORDER_ASC)
			order = "ASC";
	}
	return nullptr;
}


std::shared_ptr<http_response> resource_utils::string_to_color(const http_request& req, std::string str, int& color)
{
	if(str.size() != 7)
		return create_response::string(req, "Color '" + str + "' is not 7 characters long", 400);
	try{
		color = std::stoul(str.substr(1), nullptr, 16);
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Color '" + str + "' is not a valid hexadecimal number", 400);
	}
	return nullptr;
}
std::string resource_utils::color_to_string(int color)
{
	std::stringstream ss;
	ss << "#" << std::setfill('0') << std::setw(6) << std::hex << color;
	return ss.str();
}

std::shared_ptr<http_response> resource_utils::parse_user_id(const http_request& req, pqxx::work& tx, int& user_id)
{
	try{
		user_id = std::stoi(std::string(req.get_arg("user_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid user ID", 400);
	}
	pqxx::result r = tx.exec("SELECT user_id FROM users WHERE user_id = $1", pqxx::params(user_id));
	if(!r.size())
		return create_response::string(req, "User does not exist", 404);
	return nullptr;
}


std::shared_ptr<http_response> resource_utils::parse_server_id(const http_request& req, int user_id, pqxx::work& tx, int& server_id)
{
	try{
		server_id = std::stoi(std::string(req.get_arg("server_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid server ID", 400);
	}
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(user_id, server_id));
	if(!r.size())
		return create_response::string(req, "User is not a member of the server", 403);
	return nullptr;
}
std::shared_ptr<http_response> resource_utils::parse_server_id(const http_request& req, pqxx::work& tx, int& user_id, int& server_id)
{
	auto err = parse_session_token(req, tx, user_id);
	if(err) return err;
	return parse_server_id(req, user_id, tx, server_id);
}

std::shared_ptr<http_response> resource_utils::parse_channel_id(const http_request& req, int user_id, pqxx::work& tx, int& server_id, int& channel_id)
{
	try{
		channel_id = std::stoi(std::string(req.get_arg("channel_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid channel ID", 400);
	}

	pqxx::result r = tx.exec("SELECT channel_id, server_id FROM channels WHERE server_id IN (SELECT server_id FROM user_x_server WHERE user_id = $1) AND channel_id = $2", pqxx::params(user_id, channel_id));
	if(!r.size()){
		// Try to find a DM channel
		r = tx.exec("SELECT channel_id FROM channels WHERE (user1_id = $1 OR user2_id = $1) AND channel_id = $2", pqxx::params(user_id, channel_id));
		if(!r.size())
			return create_response::string(req, "Channel does not exist", 404);
		else
			server_id = -1;
	} else
		server_id = r[0]["server_id"].as<int>();
	return std::shared_ptr<http_response>(nullptr);
}
std::shared_ptr<http_response> resource_utils::parse_channel_id(const http_request& req, pqxx::work& tx, int& user_id, int& server_id, int& channel_id)
{
	auto err = parse_session_token(req, tx, user_id);
	if(err) return err;
	return parse_channel_id(req, user_id, tx, server_id, channel_id);
}

std::shared_ptr<http_response> resource_utils::parse_message_id(const http_request& req, int user_id, pqxx::work& tx, int& server_id, int& channel_id, int& message_id)
{
	try{
		message_id = std::stoi(std::string(req.get_arg("message_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid message ID", 400);
	}
	pqxx::result r = tx.exec("SELECT message_id, channel_id, server_id FROM messages NATURAL JOIN channels WHERE channel_id IN (SELECT channel_id FROM channels WHERE server_id IN (SELECT server_id FROM user_x_server WHERE user_id = $1)) AND message_id = $2", pqxx::params(user_id, message_id));
	if(!r.size()){
		// Try to find a direct message
		pqxx::result r = tx.exec("SELECT message_id, channel_id FROM messages NATURAL JOIN channels WHERE channel_id IN (SELECT channel_id FROM channels WHERE user1_id = $1 OR user2_id = $1) AND message_id = $2", pqxx::params(user_id, message_id));
		if(!r.size())
			return create_response::string(req, "Message does not exist", 404);
		channel_id = r[0]["channel_id"].as<int>();
		server_id = -1;
	} else {
		channel_id = r[0]["channel_id"].as<int>();
		server_id = r[0]["server_id"].is_null() ? -1 : r[0]["server_id"].as<int>();
	}
	return std::shared_ptr<http_response>(nullptr);
}
std::shared_ptr<http_response> resource_utils::parse_message_id(const http_request& req, pqxx::work& tx, int& user_id, int& server_id, int& channel_id, int& message_id)
{
	auto err = parse_session_token(req, tx, user_id);
	if(err) return err;
	return parse_message_id(req, user_id, tx, server_id, channel_id, message_id);
}

std::shared_ptr<http_response> resource_utils::parse_server_user_id(const http_request& req, int server_id, pqxx::work& tx, int& server_user_id)
{
	try{
		server_user_id = std::stoi(std::string(req.get_arg("server_user_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid server user ID", 400);
	}
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(server_user_id, server_id));
	if(!r.size())
		return create_response::string(req, "User is not a member of the server", 404);
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_server_ban_id(const http_request& req, int server_id, pqxx::work& tx, int& server_ban_id)
{
	try{
		server_ban_id = std::stoi(std::string(req.get_arg("server_ban_id")));
	} catch(std::invalid_argument& e){
		return create_response::string(req, "Invalid server ban ID", 400);
	}
	pqxx::result r = tx.exec("SELECT user_id FROM server_bans WHERE ban_id = $1 AND server_id = $2", pqxx::params(server_ban_id, server_id));
	if(!r.size())
		return create_response::string(req, "Ban does not exist", 404);
	return nullptr;
}


int resource_utils::get_channel_other_user_id(int channel_id, int user_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT user1_id, user2_id FROM channels WHERE channel_id=$1", pqxx::params(channel_id));
	int user1_id = r[0]["user1_id"].as<int>(), user2_id = r[0]["user2_id"].as<int>();
	return user_id == user1_id ? user2_id : user1_id;
}


std::shared_ptr<http_response> resource_utils::check_user_unblocked(const http_request& req, int user_from_id, int user_to_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT user1_id FROM blocked_users WHERE user1_id = $1 AND user2_id = $2", pqxx::params(user_from_id, user_to_id));
	if(r.size())
		return create_response::string(req, "This user blocked you", 403);
	r = tx.exec("SELECT user1_id FROM blocked_users WHERE user1_id = $1 AND user2_id = $2", pqxx::params(user_to_id, user_from_id));
	if(r.size())
		return create_response::string(req, "You blocked this user", 403);
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::check_server_owner(const http_request& req, int user_id, int server_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT owner_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
	if(r[0]["owner_id"].as<int>() != user_id)
		return create_response::string(req, "User is not the owner of the server", 403);
	return nullptr;
}
bool resource_utils::check_server_member(int user_id, int server_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2", pqxx::params(user_id, server_id));
	return r.size();
}
std::shared_ptr<http_response> resource_utils::check_server_member(const http_request& req, int user_id, int server_id, pqxx::work& tx)
{
	if(!check_server_member(user_id, server_id, tx))
		return create_response::string(req, "User with ID " + std::to_string(user_id) + " is not a member of the server", 403);
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_invite_id(const http_request& req, pqxx::work& tx, std::string& invite_id)
{
	invite_id = std::string(req.get_arg("invite_id"));
	pqxx::result r;
	try{
		r = tx.exec("SELECT server_id FROM server_invites WHERE invite_id = $1", pqxx::params(invite_id));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid UUID '" + invite_id + "'", 400);
	}
	if(!r.size())
		return create_response::string(req, "Server does not have this invite", 404);
	return std::shared_ptr<http_response>(nullptr);
}
std::shared_ptr<http_response> resource_utils::parse_invite_id(const http_request& req, int server_id, pqxx::work& tx, std::string& invite_id)
{
	invite_id = std::string(req.get_arg("invite_id"));
	pqxx::result r;
	try{
		r = tx.exec("SELECT server_id FROM server_invites WHERE invite_id = $1", pqxx::params(invite_id));
	} catch(pqxx::data_exception& e){
		return create_response::string(req, "Invalid UUID '" + invite_id + "'", 400);
	}
	if(!r.size() || r[0]["server_id"].as<int>() != server_id)
		return create_response::string(req, "Server does not have this invite", 404);
	return std::shared_ptr<http_response>(nullptr);
}

/* Pagination */

std::shared_ptr<http_response> resource_utils::pagination_query(const http_request& req, const config& cfg, std::string sort_column,
							pqxx::params& params, std::string& query)
{
	int start_id;
	auto err = resource_utils::parse_index(req, "start_id", start_id, 0);
	if(err) return err;
	int count;
	err = resource_utils::parse_index(req, "count", count, 0, cfg.max_get_count);
	if(err) return err;
	std::string order;
	err = resource_utils::parse_order(req, order);
	if(err) return err;

	params.append(start_id);
	size_t pri_start = params.size();
	params.append(count);

	query = " AND " + sort_column + std::string(order == "DESC" ? " <=" : " >=") + " $" + std::to_string(pri_start) + " ORDER BY " + sort_column + " " + order + " LIMIT $" + std::to_string(pri_start + 1) + " ";
	return nullptr;
}

/* Notifications */
void resource_utils::inc_notification(int channel_id, int user_id, pqxx::work& tx)
{
	tx.exec("INSERT INTO channel_notifications(channel_id, user_id) VALUES($1, $2) "
		"ON CONFLICT(channel_id, user_id) DO UPDATE "
		"SET notification_count = channel_notifications.notification_count + 1",
		pqxx::params(channel_id, user_id));
}


/* JSON */

std::shared_ptr<http_response> resource_utils::json_from_content(const http_request& req, nlohmann::json& data)
{
	try {
		data = nlohmann::json::parse(req.get_content());
	} catch(nlohmann::json::parse_error& e){
		return create_response::string(req, "Invalid JSON content", 400);
	}
	return nullptr;
}

void resource_utils::json_set_ids(nlohmann::json& data, int server_id, int channel_id)
{
	if(server_id != -1)
		data["server_id"] = server_id;
	if(channel_id != -1)
		data["channel_id"] = channel_id;
}

nlohmann::json resource_utils::user_json_from_row(const pqxx::row& r)
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

nlohmann::json resource_utils::server_json_from_row(const pqxx::row& r)
{
	nlohmann::json res = {{"id", r["server_id"].as<int>()},
				{"name", r["name"].as<std::string>()}};
	if(!r["avatar"].is_null())
		res += {"avatar", r["avatar"].as<std::string>()};
	return res;
}

nlohmann::json resource_utils::channel_json_from_row(const pqxx::row& r, int user_id)
{
	nlohmann::json res = {
		{"id", r["channel_id"].as<int>()},
		{"type", r["type"].as<int>()}
	};
	if(!r["name"].is_null())
		res += {"name", r["name"].as<std::string>()};
	if(!r["notification_count"].is_null())
		res += {"unread_messages", r["notification_count"].as<int>()};
	if(user_id > -1 && !r["user1_id"].is_null()){
		int user1_id = r["user1_id"].as<int>(), user2_id = r["user2_id"].as<int>();
		res += {"other_user_id", user1_id == user_id ? user2_id : user1_id};
	}
	return res;
}

nlohmann::json resource_utils::message_json_from_row(const pqxx::row& msg_row, const std::vector<pqxx::row>& attachment_rows)
{
	nlohmann::json r = {
		{"id", msg_row["message_id"].as<int>()},
		{"author_id", msg_row["author_id"].as<int>()},
		{"sent", msg_row["sent"].as<std::string>()},
		{"edited", msg_row["last_edited"].as<std::string>()},
		{"text", msg_row["text"].as<std::string>()},
		{"attachments", nlohmann::json::array()}
	};
	for(auto i = attachment_rows.begin(); i != attachment_rows.end(); ++i)
		r["attachments"].push_back({
			{"type", (*i)["type"].as<unsigned>()},
			{"content", (*i)["content"].as<std::string>()}
		});
	return r;
}
nlohmann::json resource_utils::message_json_from_row(const pqxx::row& msg_row, const pqxx::result& _attachment_rows)
{
	std::vector<pqxx::row> attachment_rows;
	for(auto i = _attachment_rows.begin(); i != _attachment_rows.end(); ++i)
		attachment_rows.push_back(*i);
	return message_json_from_row(msg_row, attachment_rows);
}

nlohmann::json resource_utils::invite_json_from_row(const pqxx::row& r)
{
	return {
		{"id", r["invite_id"].as<std::string>()},
		{"server_id", r["server_id"].as<int>()},
		{"expires", r["expiration_time"].is_null() ? "never" : r["expiration_time"].as<std::string>()}
	};
}
nlohmann::json resource_utils::ban_json_from_row(const pqxx::row& r)
{
	return {
		{"id", r["ban_id"].as<int>()},
		{"user", resource_utils::user_json_from_row(r)},
		{"expires", r["expiration_time"].is_null() ? "never" : r["expiration_time"].as<std::string>()}
	};
}
