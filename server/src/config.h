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
private:
	std::string db_user, db_password, db_name;
};

#endif
