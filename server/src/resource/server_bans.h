#ifndef RESOURCE_SERVER_BANS_H
#define RESOURCE_SERVER_BANS_H

#include "db/conn_pool.h"
#include "socket/main_server.h"
#include <thread>
#include <resource/base.h>

class server_bans_resource : public base_resource
{
public:
	server_bans_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	/* CONFIG PARAMETERS */
	unsigned max_get_count = 50;
	size_t cleanup_period = 3600;
private:
	db_connection_pool& pool;
	socket_main_server& sserv;

	std::thread ban_time_thr;
	static void ban_time_func(server_bans_resource& inst);
};

class server_ban_id_resource : public base_resource
{
public:
	server_ban_id_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
};


#endif
