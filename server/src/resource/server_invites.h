#ifndef RESOURCE_SERVER_INVITES_H
#define RESOURCE_SERVER_INVITES_H

#include "socket/main_server.h"
#include <thread>
#include <resource/base.h>

// Handles accepting invites
class server_invites_resource : public base_resource
{
public:
	server_invites_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

private:
	socket_main_server& sserv;

	std::thread invite_time_thr;
	static void invite_time_func(server_invites_resource& inst);
};

class server_id_invites_resource : public base_resource
{
public:
	server_id_invites_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);
};

class server_invite_id_resource : public base_resource
{
public:
	server_invite_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
};

#endif
