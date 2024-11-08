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

	unsigned min_password_length = 8;

	unsigned servers_owned_per_user = 10;
	unsigned max_channels_per_server = 50;

	unsigned server_users_max_get_count = 50;
private:
	std::string db_user, db_password, db_name;
};

#endif
