#ifndef RESOURCE_CHANNEL_MESSAGES_H
#define RESOURCE_CHANNEL_MESSAGES_H

#include "db/conn_pool.h"
#include "resource/auth.h"

class channel_messages_resource : public http_resource
{
public:
	channel_messages_resource(db_connection_pool& pool, auth_resource& auth);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);

	unsigned max_get_count = 50;
private:
	db_connection_pool& pool;
	auth_resource& auth;
};

class channel_message_id_resource : public http_resource
{
public:
	channel_message_id_resource(db_connection_pool& pool, auth_resource& auth);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
private:
	db_connection_pool& pool;
	auth_resource& auth;
};

#endif
