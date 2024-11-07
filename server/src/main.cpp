#include <pqxx/pqxx>
#include <fstream>
#include <iostream>

#include "config.h"
#include "resource/auth.h"
#include "resource/server.h"
#include "resource/user.h"
#include "resource/server_users.h"
#include "db/init.h"

int main()
{
	std::ifstream fd{"config.json"};
	config cfg;
	try {
		cfg = config{fd};
	} catch(std::logic_error& e) {
		std::cout << "Error parsing config: " << e.what() << "\n";
		return -1;
	}

	db_init(cfg.get_conn_str());
	db_connection_pool pool{cfg.get_conn_str()};

	httpserver::webserver ws = httpserver::create_webserver(cfg.listen_port)
								.use_ssl()
								.https_mem_key(cfg.https_key)
								.https_mem_cert(cfg.https_cert);
	std::cout << "Listeting on port " << cfg.listen_port << "\n";

	auth_resource auth(pool);
	auth.min_password_length = cfg.min_password_length;
	ws.register_resource("/auth", &auth);

	server_resource server(pool, auth);
	server.owned_per_user = cfg.servers_owned_per_user;
	ws.register_resource("/servers", &server);
	server_id_resource server_id(pool, auth);
	ws.register_resource("/servers/{server_id}", &server_id);
	server_users_resource server_users(pool, auth);
	server_users.max_get_count = cfg.server_users_max_get_count;
	ws.register_resource("/servers/{server_id}/users", &server_users);

	user_id_resource user_id(pool, auth);
	ws.register_resource("/users/{user_id}", &user_id);

	ws.start(true);
}
