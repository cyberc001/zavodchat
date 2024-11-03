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

	if(cfg["db_user"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_user", cfg, "string")};
	if(cfg["db_password"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_password", cfg, "string")};
	if(cfg["db_name"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("db_name", cfg, "string")};
	if(cfg["listen_port"].type() != json::value_t::number_unsigned)
		throw std::logic_error{msg_type_error("listen_port", cfg, "number_unsigned")};\
	if(cfg["https_key"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("https_key", cfg, "string")};
	if(cfg["https_cert"].type() != json::value_t::string)
		throw std::logic_error{msg_type_error("https_cert", cfg, "string")};

	db_user = cfg["db_user"].get<std::string>();
	db_password = cfg["db_password"].get<std::string>();
	db_name = cfg["db_name"].get<std::string>();
	listen_port = cfg["listen_port"].get<unsigned>();
	https_key = cfg["https_key"].get<std::string>();
	https_cert = cfg["https_cert"].get<std::string>();
}

std::string config::get_conn_str() const
{
	std::stringstream conn_str;
	conn_str << "user=" << db_user;
	conn_str << " password=" << db_password;
	conn_str << " dbname=" << db_name;
	return conn_str.str();
}
