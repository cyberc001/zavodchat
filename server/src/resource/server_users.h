#ifndef RESOURCE_SERVER_USERS_H
#define RESOURCE_SERVER_USERS_H

#include "db/conn_pool.h"
#include "socket/main_server.h"
#include <resource/base.h>

class server_users_resource : public base_resource
{
public:
	server_users_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_GET(const http_request&);

	/* CONFIG PARAMETERS */
	unsigned max_get_count = 50;
private:
	db_connection_pool& pool;
};
class server_user_id_resource : public base_resource
{
public:
	server_user_id_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};

class server_user_id_roles_resource : public base_resource
{
public:
	server_user_id_roles_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};
class server_user_role_id_resource : public base_resource
{
public:
	server_user_role_id_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};

#endif
