#ifndef RESOURCE_SERVER_H
#define RESOURCE_SERVER_H

#include "db/conn_pool.h"
#include "resource/auth.h"

class server_resource : public http_resource
{
public:
	server_resource(db_connection_pool& pool, auth_resource& auth);
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	unsigned owned_per_user = 10;
private:
	db_connection_pool& pool;
	auth_resource& auth;
};

class server_id_resource : public http_resource
{
public:
	server_id_resource(db_connection_pool& pool, auth_resource& auth);
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
	auth_resource& auth;
};

#endif
