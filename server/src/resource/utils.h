#ifndef RESOURCE_UTILS_H
#define RESOURCE_UTILS_H

#include <vector>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include <pqxx/pqxx>
#include <httpserver.hpp>
using namespace httpserver;

#include "config.h"

class create_response
{
public:
	static std::shared_ptr<http_response> string(const http_request& req, std::string str, int code);
	static std::shared_ptr<http_response> file(const http_request& req, std::string fpath);

	static void set_origins(std::vector<std::string> origins);
private:
	static void add_cors(const http_request& req, http_response& res);

	static std::vector<std::string> origins;
};

enum order_type {
	ORDER_DESC,
	ORDER_ASC
};

template<typename T>
struct array_diff
{
	array_diff(const std::vector<T>& in)
	{
		unchanged = in;
	}
	array_diff(const std::vector<T>& old, const std::vector<T>& _new)
	{
		// Using sets instead of std::find() on vectors yields O(NlogN) instead of O(N^2).
		// TODO: maybe sort arrays before storing them in channels table?
		std::unordered_set<T> s_old(old.begin(), old.end()), s_new(_new.begin(), _new.end());
		for(auto i = s_old.begin(); i != s_old.end(); ++i)
			if(!s_new.count(*i))
				removed.push_back(*i);
		for(auto i = s_new.begin(); i != s_new.end(); ++i)
			if(!s_old.count(*i))
				added.push_back(*i);
			else
				unchanged.push_back(*i);
	}

	std::vector<T> added, removed, unchanged;
};

class resource_utils
{
public:
	time_t time_now();

	/* Parsing methods
	 * They return nullptr in case of no error.
	 * */

	// Returns an error if the parameter cannot be parsed as int or violates bounds
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string arg_name, int& index);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string arg_name, int& index, int lower_bound);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string arg_name, int& index, int lower_bound, int upper_bound);

	static std::shared_ptr<http_response> parse_session_token(const http_request&, pqxx::work& tx, int& user_id);
	static std::shared_ptr<http_response> parse_timestamp(const http_request&, std::string arg_name, std::string& ts);

	static std::shared_ptr<http_response> parse_order(const http_request&, std::string& order);

	static std::shared_ptr<http_response> string_to_color(const http_request&, std::string str, int& color);
	static std::string color_to_string(int color);

	static std::shared_ptr<http_response> parse_user_id(const http_request&, pqxx::work&, int& user_id);

	static std::shared_ptr<http_response> parse_server_id(const http_request&, int user_id, pqxx::work&, int& server_id);
	static std::shared_ptr<http_response> parse_server_id(const http_request&, pqxx::work&, int& user_id, int& server_id);

	static std::shared_ptr<http_response> parse_channel_id(const http_request&, int user_id, pqxx::work&, int& server_id, int& channel_id);
	static std::shared_ptr<http_response> parse_channel_id(const http_request&, pqxx::work&, int& user_id, int& server_id, int& channel_id);

	static std::shared_ptr<http_response> parse_message_id(const http_request&, int user_id, pqxx::work&, int& server_id, int& channel_id, int& message_id);
	static std::shared_ptr<http_response> parse_message_id(const http_request&, pqxx::work&, int& user_id, int& server_id, int& channel_id,  int& message_id);

	static std::shared_ptr<http_response> parse_server_user_id(const http_request&, int server_id, pqxx::work&, int& server_user_id);
	static std::shared_ptr<http_response> parse_server_ban_id(const http_request&, int server_id, pqxx::work&, int& server_ban_id);

	// Get users that have access to this channel. If user_id != -1, don't add blocked users
	static std::vector<int> get_channel_users(int channel_id, pqxx::work&, int user_id = -1);
	// Get users that have this role.
	static std::vector<int> get_role_users(int role_id, pqxx::work&);

	static int get_channel_other_user_id(int channel_id, int user_id, pqxx::work&);

	// Check if user_to_id has user_from_id unblocked.
	static std::shared_ptr<http_response> check_user_unblocked(const http_request&, int user_from_id, int user_to_id, pqxx::work&);

	// Checks if server's owner_id == user_id.
	static bool check_server_owner(int user_id, int server_id, pqxx::work&);
	static std::shared_ptr<http_response> check_server_owner(const http_request&, int user_id, int server_id, pqxx::work&);

	// Separate check for user being a member of a server. Used when user is not the one that puts a request.
	static bool check_server_member(int user_id, int server_id, pqxx::work&);
	static std::shared_ptr<http_response> check_server_member(const http_request&, int user_id, int server_id, pqxx::work&);
	static bool check_channel_member(int user_id, int channel_id, int server_id, pqxx::work&);

	static std::shared_ptr<http_response> parse_invite_id(const http_request&, int server_id, pqxx::work&, std::string& invite_id);
	// Same as above, but also checks if server has the invite
	static std::shared_ptr<http_response> parse_invite_id(const http_request&, pqxx::work&, std::string& invite_id);


	static std::string int_array_to_string(const std::vector<int>&);

	static std::unordered_set<int> get_valid_user_ids(const std::vector<int>&, pqxx::work&, int server_id);
	static std::vector<int> get_valid_user_ids_vector(const std::vector<int>&, pqxx::work&, int server_id);
	static std::unordered_set<int> get_valid_role_ids(const std::vector<int>&, pqxx::work&, int server_id);
	static std::vector<int> get_valid_role_ids_vector(const std::vector<int>&, pqxx::work&, int server_id);

	/* Queries */
	static std::shared_ptr<http_response> pagination_query(const http_request&, const config&, std::string sort_column,
							pqxx::params& params, std::string& query, std::string* order = nullptr);
	static std::string pagination_query(const http_request&);
	static std::string no_blocked_users_query(int user_id_param_i, std::string user_id_column = "user_id");

	/* Other */
	static std::vector<int> parse_psql_int_array(const pqxx::field& f);
};

#endif
