#ifndef RESOURCE_FRIEND_H
#define RESOURCE_FRIEND_H

#include "socket/main_server.h"
#include "socket/vc_server.h"
#include <resource/base.h>

class friends_resource : public base_resource
{
public:
	friends_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_vc_server& vcserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
private:
	socket_vc_server& vcserv;
};

class friend_requests_resource : public base_resource
{
public:
	friend_requests_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
};

class friends_id_resource : public base_resource
{
public:
	friends_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv);

	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	socket_main_server& sserv;
};

#endif
