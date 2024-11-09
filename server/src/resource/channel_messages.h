#ifndef RESOURCE_CHANNEL_MESSAGES_H
#define RESOURCE_CHANNEL_MESSAGES_H

#include "db/conn_pool.h"
#include "resource/auth.h"

class channel_messages_resource : public http_resource
{
public:
	channel_messages_resource(db_connection_pool& pool, auth_resource& auth);

	std::shared_ptr<http_response> render_GET(const http_request&);

	unsigned max_get_count = 50;
private:
	db_connection_pool& pool;
	auth_resource& auth;
};

#endif
