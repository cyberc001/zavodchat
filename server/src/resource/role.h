#ifndef RESOURCE_ROLES_H
#define RESOURCE_ROLES_H

#include "socket/main_server.h"
#include <resource/base.h>

class server_roles_resource : public base_resource
{
public:
	server_roles_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

private:
	int get_next_id(const std::vector<nlohmann::json>& list, int i);
	socket_main_server& sserv;
};

#endif
