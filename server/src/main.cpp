#include <pqxx/pqxx>
#include <fstream>
#include <iostream>

#include "config.h"
#include "resource/auth.h"
#include "db/conn_pool.h"
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
	connection_pool pool{cfg.get_conn_str()};

	httpserver::webserver ws = httpserver::create_webserver(cfg.listen_port)
								.use_ssl()
								.https_mem_key(cfg.https_key)
								.https_mem_cert(cfg.https_cert);
	std::cout << "Listeting on port " << cfg.listen_port << "\n";
	auth_resource auth(pool);
	ws.register_resource("/auth", &auth);
	ws.start(true);

	/*pqxx::connection cx{cfg.get_conn_str()};
	pqxx::work tx{cx};

	tx.exec("CREATE TABLE test(name varchar(255));");
	tx.commit();*/
}
