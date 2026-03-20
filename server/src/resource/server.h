#ifndef RESOURCE_SERVER_H
#define RESOURCE_SERVER_H

#include "socket/main_server.h"
#include <resource/base.h>

class server_resource : public base_resource
{
public:
	server_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);
};

class server_id_resource : public base_resource
{
public:
	server_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);

private:
	socket_main_server& sserv;
};

#endif
