#ifndef RESOURCE_UTILS_H
#define RESOURCE_UTILS_H

#include <nlohmann/json.hpp>

#include <pqxx/pqxx>
#include <httpserver.hpp>
using namespace httpserver;

enum order_type {
	ORDER_DESC,
	ORDER_ASC
};

class resource_utils
{
public:
	time_t time_now();

	/* Parsing */
	// Returns an error if the parameter cannot be parsed as int or violates bounds
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index, int lower_bound);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index, int lower_bound, int upper_bound);

	static std::shared_ptr<http_response> parse_session_token(const http_request&, pqxx::work& tx, int& user_id);

	// Checks if the server is accessible to the user. Returns nullptr if there wasn't an error
	static std::shared_ptr<http_response> parse_server_id(const http_request&, int user_id, pqxx::work&, int& server_id);
	// Same as above, but derives user_id from supplied token
	static std::shared_ptr<http_response> parse_server_id(const http_request&, pqxx::work&, int& user_id, int& server_id);

	// Checks if server's owner_id == user_id. Returns nullptr if it's true
	static std::shared_ptr<http_response> check_server_owner(int user_id, int server_id, pqxx::work&);
	// Checks if channel is in the server
	static std::shared_ptr<http_response> parse_channel_id(const http_request&, int server_id, pqxx::work&, int& channel_id);
	// Checks if message is in the channel
	static std::shared_ptr<http_response> parse_message_id(const http_request&, int channel_id, pqxx::work&, int& message_id);

	static std::shared_ptr<http_response> parse_invite_id(const http_request&, int server_id, pqxx::work&, std::string& invite_id);
	// Same as above, but also checks if server has the invite
	static std::shared_ptr<http_response> parse_invite_id(const http_request&, pqxx::work&, std::string& invite_id);

	/* JSON */
	static nlohmann::json user_json_from_row(const pqxx::row&& r);
	static nlohmann::json server_json_from_row(const pqxx::row&& r);
	static nlohmann::json channel_json_from_row(const pqxx::row&& r);
	static nlohmann::json message_json_from_row(const pqxx::row&& r);
	static nlohmann::json invite_json_from_row(const pqxx::row&& r);
};

#endif
