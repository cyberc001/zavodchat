#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include "resource/utils.h"

#define JSON_GET_STRING(_json, key)\
	if((_json)[#key].type() != nlohmann::json::value_t::string)\
		return create_response::string(req, "'" #key "' should be string", 400);\
	std::string key = (_json)[#key].get<std::string>();
#define JSON_GET_UNSIGNED(_json, key)\
	if((_json)[#key].type() != nlohmann::json::value_t::number_unsigned)\
		return create_response::string(req, "'" #key "' should be unsigned integer", 400);\
	unsigned key = (_json)[#key].get<unsigned>();


class json_utils
{
public:
	static std::shared_ptr<http_response> from_content(const http_request&, nlohmann::json&);

	template<typename T>
	static std::shared_ptr<http_response> get_array(const http_request& req, nlohmann::json& _json, nlohmann::json::value_t _type, std::string key,
								std::vector<T>& out)
	{
		if(!_json[key].is_array())
			return create_response::string(req, "'" + key + "' is not an array", 400);
	
		auto& arr = _json[key];
		out.reserve(arr.size());
		for(auto val = arr.begin(); val != arr.end(); ++val){
			if(val->type() != _type)
				return create_response::string(req, "'" + key + "' array has a value with wrong type", 400);
			out.push_back(val->get<T>());
		}

		return nullptr;
	}

	static void set_ids(nlohmann::json& data, int server_id, int channel_id = -1);

	static nlohmann::json user_from_row(const pqxx::row& r);
	static nlohmann::json server_from_row(const pqxx::row& r, bool notifications = true);
	// user_id > -1 -> could be a private channel (in that case, attempt to write to other_user_id field)
	static nlohmann::json channel_from_row(const pqxx::row& r, bool has_notifications = false, int user_id = -1);

	static nlohmann::json message_from_row(const pqxx::row& msg_row, const std::vector<pqxx::row>& attachment_rows);
	static nlohmann::json message_from_row(const pqxx::row& msg_row, const pqxx::result& attachment_rows);

	static nlohmann::json invite_from_row(const pqxx::row& r);
	static nlohmann::json ban_from_row(const pqxx::row& r);
};

#endif
