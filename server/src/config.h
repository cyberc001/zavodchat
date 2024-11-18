#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>

class config
{
public:
	config();
	config(std::ifstream& fd);

	std::string get_conn_str() const;

	unsigned listen_port;
	std::string https_key, https_cert;

	unsigned min_username_length = 2;
	unsigned min_password_length = 8;

	size_t session_lifetime = 1800;

	size_t cleanup_period = 3600;
	unsigned max_get_count = 50;

	unsigned servers_owned_per_user = 10;
	unsigned max_channels_per_server = 50;

private:
	std::string db_user, db_password, db_name;
};

#endif
