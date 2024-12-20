#ifndef RESOURCE_SERVER_CHANNELS_H
#define RESOURCE_SERVER_CHANNELS_H

#include "db/conn_pool.h"

#include <httpserver.hpp>
using namespace httpserver;

enum channel_type {
	CHANNEL_TEXT,
	CHANNEL_VOICE
};

class server_channel_resource : public http_resource
{
public:
	server_channel_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	unsigned max_per_server = 50;

private:
	db_connection_pool& pool;
};

class server_channel_id_resource : public http_resource
{
public:
	server_channel_id_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
};

#endif
