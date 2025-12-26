#ifndef RESOURCE_UTILS_H
#define RESOURCE_UTILS_H

#include <vector>

#include <nlohmann/json.hpp>

#include <pqxx/pqxx>
#include <httpserver.hpp>
using namespace httpserver;

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

	static std::shared_ptr<http_response> string_to_color(const http_request&, std::string str, int& color);
	static std::string color_to_string(int color);

	// Checks if the server is accessible to the user.
	static std::shared_ptr<http_response> parse_server_id(const http_request&, int user_id, pqxx::work&, int& server_id);
	// Same as above, but derives user_id from supplied token
	static std::shared_ptr<http_response> parse_server_id(const http_request&, pqxx::work&, int& user_id, int& server_id);

	static std::shared_ptr<http_response> parse_server_user_id(const http_request&, int server_id, pqxx::work&, int& server_user_id);

	static std::shared_ptr<http_response> parse_server_ban_id(const http_request&, int server_id, pqxx::work&, int& server_ban_id);

	// Checks if server's owner_id == user_id.
	static std::shared_ptr<http_response> check_server_owner(const http_request&, int user_id, int server_id, pqxx::work&);
	// Separate check for user being a member of a server. Used when user is not the one that puts a request.
	static bool check_server_member(int user_id, int server_id, pqxx::work&);
	static std::shared_ptr<http_response> check_server_member(const http_request&, int user_id, int server_id, pqxx::work&);

	// Checks if channel is in the server
	static std::shared_ptr<http_response> parse_channel_id(const http_request&, int server_id, pqxx::work&, int& channel_id);
	// Checks if message is in the channel
	static std::shared_ptr<http_response> parse_message_id(const http_request&, int channel_id, pqxx::work&, int& message_id);

	static std::shared_ptr<http_response> parse_invite_id(const http_request&, int server_id, pqxx::work&, std::string& invite_id);
	// Same as above, but also checks if server has the invite
	static std::shared_ptr<http_response> parse_invite_id(const http_request&, pqxx::work&, std::string& invite_id);

	/* JSON */
	static std::shared_ptr<http_response> json_from_content(const http_request&, nlohmann::json&);

	static void json_set_ids(nlohmann::json& data, int server_id);
	static void json_set_ids(nlohmann::json& data, int server_id, int channel_id);

	static nlohmann::json user_json_from_row(const pqxx::row&& r);
	static nlohmann::json server_json_from_row(const pqxx::row&& r);
	static nlohmann::json channel_json_from_row(const pqxx::row&& r);

	static nlohmann::json message_json_from_row(const pqxx::row&& r);
	static nlohmann::json message_update_json_from_row(const pqxx::row&& r);

	static nlohmann::json invite_json_from_row(const pqxx::row&& r);
};

#endif
