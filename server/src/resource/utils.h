#ifndef RESOURCE_UTILS_H
#define RESOURCE_UTILS_H

#include "resource/auth.h"
#include <nlohmann/json.hpp>

class resource_utils
{
public:
	/* Parsing */
	// Returns an error if the parameter cannot be parsed as int or violates bounds
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index, int lower_bound);
	static std::shared_ptr<http_response> parse_index(const http_request&, std::string header_name, int& index, int lower_bound, int upper_bound);
	// Checks if the server is accessible to the user. Returns nullptr if there wasn't an error
	static std::shared_ptr<http_response> parse_server_id(const http_request&, int user_id, pqxx::work&, int& server_id);
	// Same as above, but derives user_id from supplied token
	static std::shared_ptr<http_response> parse_server_id(const http_request&, auth_resource& auth, pqxx::work&, int& user_id, int& server_id);
	// Checks if server's owner_id == user_id. Returns nullptr if it's true
	static std::shared_ptr<http_response> check_server_owner(int user_id, int server_id, pqxx::work&);

	/* JSON */
	static nlohmann::json user_json_from_row(const pqxx::row&& r);
	static nlohmann::json server_json_from_row(const pqxx::row&& r);
	static nlohmann::json server_json_from_row(const pqxx::row&& r, pqxx::work&); // also retreives channel and user count
	static nlohmann::json channel_json_from_row(const pqxx::row&& r);
};

#endif
