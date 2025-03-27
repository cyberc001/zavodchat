#ifndef RESOURCE_ROLES_H
#define RESOURCE_ROLES_H

#include "db/conn_pool.h"
#include "socket/main_server.h"

#include <httpserver.hpp>
using namespace httpserver;

class server_roles_resource : public http_resource
{
public:
	server_roles_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	unsigned max_per_server = 50;
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};

class server_role_id_resource : public http_resource
{
};

#endif
