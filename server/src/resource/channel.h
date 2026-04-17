#ifndef RESOURCE_CHANNEL_H
#define RESOURCE_CHANNEL_H

#include "socket/main_server.h"
#include "socket/vc_server.h"
#include <resource/base.h>

enum channel_type {
	CHANNEL_TEXT = 0,
	CHANNEL_VOICE
};

class server_channel_resource : public base_resource
{
public:
	server_channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
					socket_main_server& sserv, socket_vc_server& vcserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

private:
	socket_main_server& sserv;
	socket_vc_server& vcserv;
};

class channel_resource : public base_resource
{
public:
	channel_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv, socket_vc_server& vcserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	socket_main_server& sserv;
	socket_vc_server& vcserv;
};

class channel_user_id_resource : public base_resource
{
public:
	channel_user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_vc_server& vcserv);

	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	socket_vc_server& vcserv;
};

class channel_roles_resource : public base_resource
{
public:
	channel_roles_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv);

	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	socket_main_server& sserv;
};

#endif
