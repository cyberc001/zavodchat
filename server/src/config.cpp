#include "config.h"
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace nlohmann;

std::string msg_type_error(std::string key, json& cfg, std::string expected)
{
	std::stringstream ss;
	ss << "Invalid type for '" << key << "'. Expected " << expected << ", got " << cfg[key].type_name();
	return ss.str();
}

config::config(){}

#define CHECK_IF_STRING(param){\
	if(cfg[#param].type() != json::value_t::string)\
		throw std::logic_error{msg_type_error(#param, cfg, "string")};\
}
#define GET_IF_UNSIGNED(param){\
	if(cfg[#param].is_number_unsigned())\
		(param) = cfg[#param].get<unsigned>();\
}
#define GET_IF_STRING(param){\
	if(cfg[#param].is_string())\
		(param) = cfg[#param].get<std::string>();\
}


config::config(std::ifstream& fd)
{
	json cfg = json::parse(fd);

	CHECK_IF_STRING(db_host)
	CHECK_IF_STRING(db_user)
	CHECK_IF_STRING(db_password)
	CHECK_IF_STRING(db_name)

	CHECK_IF_STRING(https_key)
	CHECK_IF_STRING(https_cert)

	db_host = cfg["db_host"].get<std::string>();
	GET_IF_UNSIGNED(db_port)
	db_user = cfg["db_user"].get<std::string>();
	db_password = cfg["db_password"].get<std::string>();
	db_name = cfg["db_name"].get<std::string>();

	GET_IF_UNSIGNED(https_port)
	GET_IF_UNSIGNED(ws_port)
	GET_IF_UNSIGNED(ws_vc_port)

	GET_IF_STRING(user_avatar_path)
	GET_IF_STRING(server_avatar_path)
	GET_IF_STRING(file_storage_path)

	GET_IF_STRING(rtc_addr)
	GET_IF_UNSIGNED(rtc_port)

	https_key = cfg["https_key"].get<std::string>();
	https_cert = cfg["https_cert"].get<std::string>();

	GET_IF_UNSIGNED(min_username_length)
	GET_IF_UNSIGNED(min_password_length)

	if(cfg["session_lifetime"].is_number_unsigned())
		session_lifetime = cfg["session_lifetime"].get<size_t>();
	if(cfg["cleanup_period"].is_number_unsigned())
		cleanup_period = cfg["cleanup_period"].get<size_t>();

	if(cfg["file_storage_size"].is_number_unsigned())
		file_storage_size = cfg["file_storage_size"].get<size_t>();

	GET_IF_UNSIGNED(max_get_count)

	GET_IF_UNSIGNED(servers_owned_per_user)
	GET_IF_UNSIGNED(max_channels_per_server)
	GET_IF_UNSIGNED(max_roles_per_server)

	GET_IF_UNSIGNED(max_video_bitrate)

	// create directories for file storage
	std::filesystem::create_directories(user_avatar_path);
	std::filesystem::create_directories(server_avatar_path);
	std::filesystem::create_directories(file_storage_path);
}

std::string config::get_conn_str() const
{
	std::stringstream conn_str;
	conn_str << "host=" << db_host;
	conn_str << " port=" << db_port;
	conn_str << " user=" << db_user;
	conn_str << " password=" << db_password;
	conn_str << " dbname=" << db_name;
	return conn_str.str();
}
