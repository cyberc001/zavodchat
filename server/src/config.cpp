#include "config.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

std::string msg_type_error(std::string key, json& cfg, std::string expected)
{
	std::stringstream ss;
	ss << "Invalid type for '" << key << "'. Expected " << expected << ", got " << cfg[key].type_name();
	return ss.str();
}

config::config(){}
config::config(std::ifstream& fd)
{
	json cfg = json::parse(fd);

	if(cfg["db_host"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_host", cfg, "string")};
	if(cfg["db_user"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_user", cfg, "string")};
	if(cfg["db_password"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_password", cfg, "string")};
	if(cfg["db_name"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_name", cfg, "string")};

	if(cfg["https_key"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("https_key", cfg, "string")};
	if(cfg["https_cert"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("https_cert", cfg, "string")};

	db_host = cfg["db_host"].get<std::string>();
	if(cfg["db_port"].is_number_unsigned())
		db_port = cfg["db_port"].get<unsigned>();
	db_user = cfg["db_user"].get<std::string>();
	db_password = cfg["db_password"].get<std::string>();
	db_name = cfg["db_name"].get<std::string>();

	if(cfg["https_port"].is_number_unsigned())
		https_port = cfg["https_port"].get<unsigned>();
	if(cfg["ws_port"].is_number_unsigned())
		ws_port = cfg["ws_port"].get<unsigned>();
	if(cfg["ws_vc_port"].is_number_unsigned())
		ws_vc_port = cfg["ws_vc_port"].get<unsigned>();
	if(cfg["rtc_port"].is_number_unsigned())
		rtc_port = cfg["rtc_port"].get<unsigned>();

	https_key = cfg["https_key"].get<std::string>();
	https_cert = cfg["https_cert"].get<std::string>();

	if(cfg["min_username_length"].is_number_unsigned())
		min_username_length = cfg["min_username_length"].get<unsigned>();
	if(cfg["min_password_length"].is_number_unsigned())
		min_password_length = cfg["min_password_length"].get<unsigned>();

	if(cfg["session_lifetime"].is_number_unsigned())
		session_lifetime = cfg["session_lifetime"].get<size_t>();

	if(cfg["cleanup_period"].is_number_unsigned())
		cleanup_period = cfg["cleanup_period"].get<size_t>();
	if(cfg["max_get_count"].is_number_unsigned())
		max_get_count = cfg["max_get_count"].get<unsigned>();

	if(cfg["servers_owned_per_user"].is_number_unsigned())
		servers_owned_per_user = cfg["servers_owned_per_user"].get<unsigned>();
	if(cfg["max_channels_per_server"].is_number_unsigned())
		max_channels_per_server = cfg["max_channels_per_server"].get<unsigned>();
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
