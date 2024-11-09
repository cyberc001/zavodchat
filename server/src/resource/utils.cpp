#include "resource/utils.h"

/* Parsing */

std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string header_name, int& index)
{
	try{
		index = std::stoi(std::string(req.get_header(header_name)));
	} catch(std::invalid_argument& e){
		return std::shared_ptr<http_response>(new string_response("Couldn't parse '" + header_name + "', got: " + std::string(req.get_header(header_name)), 400));
	}

	return std::shared_ptr<http_response>(nullptr);
}
std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string header_name, int& index, int lower_bound)
{
	auto err = parse_index(req, header_name, index);
	if(err) return err;

	if(index < lower_bound)
		return std::shared_ptr<http_response>(new string_response(header_name + " is invalid: " + std::to_string(index) + " is below " + std::to_string(lower_bound), 403));

	return std::shared_ptr<http_response>(nullptr);
}

std::shared_ptr<http_response> resource_utils::parse_index(const http_request& req, std::string header_name, int& index, int lower_bound, int upper_bound)
{
	auto err = parse_index(req, header_name, index);
	if(err) return err;

	if(index < lower_bound)
		return std::shared_ptr<http_response>(new string_response(header_name + " is invalid: " + std::to_string(index) + " is below " + std::to_string(lower_bound), 403));
	if(index > upper_bound)
		return std::shared_ptr<http_response>(new string_response(header_name + " is invalid: " + std::to_string(index) + " is above " + std::to_string(upper_bound), 403));

	return std::shared_ptr<http_response>(nullptr);
}

std::shared_ptr<http_response> resource_utils::parse_server_id(const http_request& req, int user_id, pqxx::work& tx, int& server_id)
{
	try{
		server_id = std::stoi(std::string(req.get_arg("server_id")));
	} catch(std::invalid_argument& e){
		return std::shared_ptr<http_response>(new string_response("Invalid server ID", 400));
	}
	pqxx::result r = tx.exec_params("SELECT user_id FROM user_x_server WHERE user_id = $1 AND server_id = $2", user_id, server_id);
	if(!r.size())
		return std::shared_ptr<http_response>(new string_response("User is not a member of the server", 403));
	return nullptr;
}
std::shared_ptr<http_response> resource_utils::parse_server_id(const http_request& req, auth_resource& auth, pqxx::work& tx, int& user_id, int& server_id)
{
	session_token token;
	auto err = auth.parse_session_token(req, token);
	if(err) return err;
	user_id = auth.sessions[token];
	return parse_server_id(req, user_id, tx, server_id);
}
std::shared_ptr<http_response> resource_utils::check_server_owner(int user_id, int server_id, pqxx::work& tx)
{
	pqxx::result r = tx.exec_params("SELECT owner_id FROM servers WHERE server_id = $1", server_id);
	if(r[0]["owner_id"].as<int>() != user_id)
		return std::shared_ptr<http_response>(new string_response("User is not the owner of the server", 403));
	return nullptr;
}

std::shared_ptr<http_response> resource_utils::parse_channel_id(const http_request& req, int server_id, pqxx::work& tx, int& channel_id)
{
	try{
		channel_id = std::stoi(std::string(req.get_arg("channel_id")));
	} catch(std::invalid_argument& e){
		return std::shared_ptr<http_response>(new string_response("Invalid server ID", 400));
	}
	pqxx::result r = tx.exec_params("SELECT server_id FROM channels WHERE channel_id = $1", channel_id);
	if(!r.size())
		return std::shared_ptr<http_response>(new string_response("Channel doesn't exist", 404));
	if(r[0]["server_id"].as<int>() != server_id)
		return std::shared_ptr<http_response>(new string_response("Channel does not belong to the server", 403));
	return std::shared_ptr<http_response>(nullptr);
}

/* JSON */

nlohmann::json resource_utils::user_json_from_row(const pqxx::row&& r)
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
nlohmann::json resource_utils::server_json_from_row(const pqxx::row&& r)
{
	nlohmann::json res = {{"id", r["server_id"].as<int>()},
				{"name", r["name"].as<std::string>()}};
	if(!r["avatar"].is_null())
		res += {"avatar", r["avatar"].as<std::string>()};
	return res;
}
nlohmann::json resource_utils::server_json_from_row(const pqxx::row&& r, pqxx::work& tx)
{
	nlohmann::json res = {{"id", r["server_id"].as<int>()},
				{"name", r["name"].as<std::string>()}};
	if(!r["avatar"].is_null())
		res += {"avatar", r["avatar"].as<std::string>()};

	pqxx::result pr = tx.exec_params("SELECT server_id FROM channels WHERE server_id = $1", res["id"].get<int>());
	res += {"channel_count", pr.size()};
	pr = tx.exec_params("SELECT server_id FROM user_x_server WHERE server_id = $1", res["id"].get<int>());
	res += {"user_count", pr.size()};
	return res;
}
nlohmann::json resource_utils::channel_json_from_row(const pqxx::row&& r)
{
	return {
		{"id", r["channel_id"].as<int>()},
		{"name", r["name"].as<std::string>()},
		{"type", r["type"].as<int>()}
	};
}
