#ifndef RESOURCE_SERVER_H
#define RESOURCE_SERVER_H

#include "db/conn_pool.h"
#include "resource/auth.h"

class server_resource : public http_resource
{
public:
	server_resource(connection_pool& pool, auth_resource& auth);
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	unsigned owned_per_user = 2;
private:
	connection_pool& pool;
	auth_resource& auth;
};

#endif
