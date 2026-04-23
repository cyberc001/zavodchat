#include "resource/utils.h"
#include "resource/role_utils.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sstream>
#include <iosfwd>

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

	pqxx::result r = tx.exec("SELECT channel_id, server_id, wl_users, wl_roles FROM channels "
				 "WHERE server_id IN (SELECT server_id FROM user_x_server WHERE user_id = $1) "
				 "AND channel_id = $2",
				 pqxx::params(user_id, channel_id));
	if(!r.size()){
		// Try to find a DM channel
		r = tx.exec("SELECT channel_id FROM channels WHERE (user1_id = $1 OR user2_id = $1) AND channel_id = $2", pqxx::params(user_id, channel_id));
		if(!r.size())
			return create_response::string(req, "Channel does not exist", 404);
		else
			server_id = -1;
	} else {
		server_id = r[0]["server_id"].as<int>();
		if(!check_channel_member(user_id, channel_id, server_id, tx))
			return create_response::string(req, "Channel does not exist", 404);
	}
	return nullptr;
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


std::vector<int> resource_utils::get_channel_users(int channel_id, pqxx::work& tx, int user_id)
{
	std::vector<int> out;
	pqxx::result r = tx.exec("SELECT server_id, user1_id, user2_id, wl_users, wl_roles FROM channels WHERE channel_id = $1",
				 pqxx::params(channel_id));
	if(r[0]["server_id"].is_null()){
		out.push_back(r[0]["user1_id"].as<int>());
		out.push_back(r[0]["user2_id"].as<int>());
	} else {
		int server_id = r[0]["server_id"].as<int>();
		std::vector<int> wl_users = resource_utils::parse_psql_int_array(r[0]["wl_users"]),
				 wl_roles = resource_utils::parse_psql_int_array(r[0]["wl_roles"]);

		std::string wl_check, having;
		if(wl_users.size() > 0 || wl_roles.size() > 0){
			wl_check = "AND (";
			if(wl_users.size() > 0)
				wl_check += "user_id IN (" + resource_utils::int_array_to_string(wl_users) + ")";
			if(wl_roles.size() > 0)
				wl_check += std::string(wl_users.size() > 0 ? " OR " : "") + "role_id IN (" + resource_utils::int_array_to_string(wl_roles) + ")";
			wl_check += ")";
		}

		pqxx::params pr(server_id);
		if(user_id > -1)
			pr.append(user_id);
		r = tx.exec("SELECT user_id FROM user_x_server "
			    "WHERE server_id = $1 " + wl_check + 
			    (user_id > -1 ? " AND NOT EXISTS(SELECT user2_id FROM blocked_users WHERE user1_id = user_id AND user2_id = $2)" : "") +
			    " GROUP BY user_id" + having,
			    pr);
		for(size_t i = 0; i < r.size(); ++i)
			out.push_back(r[i]["user_id"].as<int>());
	}
	return out;
}
std::vector<int> resource_utils::get_role_users(int role_id, pqxx::work& tx)
{
	std::vector<int> out;
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE role_id = $1",
				pqxx::params(role_id));
	for(size_t i = 0; i < r.size(); ++i)
		out.push_back(r[i]["user_id"].as<int>());
	return out;
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

bool resource_utils::check_server_owner(int user_id, int server_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec("SELECT owner_id FROM servers WHERE server_id = $1", pqxx::params(server_id));
	return r[0]["owner_id"].as<int>() == user_id;
}
std::shared_ptr<http_response> resource_utils::check_server_owner(const http_request& req, int user_id, int server_id, pqxx::work& tx)
{
	if(check_server_owner(user_id, server_id, tx))
		return nullptr;
	return create_response::string(req, "User is not the owner of the server", 403);
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
bool resource_utils::check_channel_member(int user_id, int channel_id, int server_id, pqxx::work& tx)
{
	if(!role_utils::check_permission(tx, server_id, user_id,
						role_utils::perms1, PERM1_MANAGE_CHANNELS)){
		pqxx::result ch = tx.exec("SELECT wl_users, wl_roles FROM channels WHERE channel_id = $1",
					     pqxx::params(channel_id));
		std::vector<int> wl_users = parse_psql_int_array(ch[0]["wl_users"]);
		if(wl_users.size() > 0 && std::find(wl_users.begin(), wl_users.end(), user_id) != wl_users.end())
			return true;

		std::vector<int> wl_roles = parse_psql_int_array(ch[0]["wl_roles"]);
		if(wl_users.size() == 0 && wl_roles.size() == 0)
			return true;
		pqxx::result roles = tx.exec("SELECT role_id FROM user_x_server WHERE user_id = $1 AND server_id = $2",
					     pqxx::params(user_id, server_id));
		for(size_t i = 0; i < roles.size(); ++i)
			if(std::find(wl_roles.begin(), wl_roles.end(), roles[i]["role_id"].as<int>()) != wl_roles.end())
				return true;

		return false;
	}
	return true;
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


std::string resource_utils::int_array_to_string(const std::vector<int>& in)
{
	std::stringstream ss;
	bool first = true;
	for(auto i = in.begin(); i != in.end(); ++i){
		if(!first)
			ss << ",";
		first = false;
		ss << *i;
	}
	return ss.str();
}

#define __GET_VALID_USER_IDS(container_t, insert_f){\
	container_t<int> _out;\
	if(in.size()){\
		pqxx::result r = tx.exec("SELECT DISTINCT ON(user_id) user_id FROM user_x_server "\
					 "WHERE server_id = $1 AND user_id IN (" + int_array_to_string(in) + ")",\
					 pqxx::params(server_id));\
		for(size_t i = 0; i < r.size(); ++i)\
			_out.insert_f(r[i]["user_id"].as<int>());\
	}\
	return _out;\
}
std::unordered_set<int> resource_utils::get_valid_user_ids(const std::vector<int>& in, pqxx::work& tx, int server_id)
	__GET_VALID_USER_IDS(std::unordered_set, insert)
std::vector<int> resource_utils::get_valid_user_ids_vector(const std::vector<int>& in, pqxx::work& tx, int server_id)
	__GET_VALID_USER_IDS(std::vector, push_back)

#define __GET_VALID_ROLE_IDS(container_t, insert_f){\
	container_t<int> _out;\
	if(in.size()){\
		pqxx::result r = tx.exec("SELECT DISTINCT ON(role_id) role_id FROM roles "\
					 "WHERE server_id = $1 AND role_id IN (" + int_array_to_string(in) + ")",\
					 pqxx::params(server_id));\
		for(size_t i = 0; i < r.size(); ++i)\
			_out.insert_f(r[i]["role_id"].as<int>());\
	}\
	return _out;\
}
std::unordered_set<int> resource_utils::get_valid_role_ids(const std::vector<int>& in, pqxx::work& tx, int server_id)
	__GET_VALID_ROLE_IDS(std::unordered_set, insert)
std::vector<int> resource_utils::get_valid_role_ids_vector(const std::vector<int>& in, pqxx::work& tx, int server_id)
	__GET_VALID_ROLE_IDS(std::vector, push_back)


/* Queries */

std::shared_ptr<http_response> resource_utils::pagination_query(const http_request& req, const config& cfg, std::string sort_column,
							pqxx::params& params, std::string& query, std::string* order_out)
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

	if(order_out)
		*order_out = " ORDER BY " + sort_column + " " + order;

	query = " AND " + sort_column + std::string(order == "DESC" ? " <=" : " >=") + " $" + std::to_string(pri_start) + " ORDER BY " + sort_column + " " + order + " LIMIT $" + std::to_string(pri_start + 1) + " ";
	return nullptr;
}

std::string resource_utils::no_blocked_users_query(int user_id_param_i, std::string user_id_column)
{
	return "NOT EXISTS(SELECT user2_id FROM blocked_users WHERE user1_id = $" +
	       std::to_string(user_id_param_i) + " AND user2_id = " + user_id_column + ")";
}


/* Other */

std::vector<int> resource_utils::parse_psql_int_array(const pqxx::field& f)
{
	std::vector<int> arr;
	pqxx::array_parser parser = f.as_array();
	auto obj = parser.get_next();
	while(obj.first != pqxx::array_parser::juncture::done){
		if(obj.first == pqxx::array_parser::juncture::string_value)
			arr.push_back(std::strtol(obj.second.c_str(), nullptr, 10));
		obj = parser.get_next();
	}
	return arr;
}
