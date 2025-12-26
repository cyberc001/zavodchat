#ifndef RESOURCE_ROLES_H
#define RESOURCE_ROLES_H

#include "db/conn_pool.h"
#include "socket/main_server.h"
#include <resource/base.h>

class server_roles_resource : public base_resource
{
public:
	server_roles_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	/* CONFIG PARAMETERS */
	unsigned max_per_server = 50;
private:
	int get_next_id(const std::vector<nlohmann::json>& list, int i);

	db_connection_pool& pool;
	socket_main_server& sserv;
};

#endif
