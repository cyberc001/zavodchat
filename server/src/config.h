#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <fstream>

class config
{
public:
	config();
	config(std::ifstream& fd);

	std::string get_conn_str() const;

	unsigned https_port = 443, ws_port = 444, ws_vc_port = 445;
	std::vector<std::string> origins;
	
	std::string rtc_addr = "127.0.0.1";
	unsigned rtc_port = 50000;

	std::string https_key, https_cert;

	std::string user_avatar_path = "/data/avatar/user/",
			server_avatar_path = "/data/avatar/server/",
			file_storage_path = "/data/upload/";

	unsigned min_username_length = 2;
	unsigned min_password_length = 8;

	size_t session_lifetime = 1800;
	size_t cleanup_period = 3600;
	unsigned sessions_per_user = 8;

	size_t file_storage_size = 10737418240;

	unsigned max_get_count = 50;

	unsigned servers_owned_per_user = 10;
	unsigned max_channels_per_server = 50;
	unsigned max_roles_per_server = 50;

	unsigned max_video_bitrate = 10240000;

private:
	std::string db_host; unsigned db_port = 5432;
	std::string db_user, db_password, db_name;
};

#endif
