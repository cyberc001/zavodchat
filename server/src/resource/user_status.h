#ifndef USER_STATUS_H
#define USER_STATUS_H

#include "db/conn_pool.h"
#include <thread>

#include <httpserver.hpp>
using namespace httpserver;

#define STATUS_OFFLINE		0
#define STATUS_ONLINE		1
#define STATUS_AWAY		2
#define STATUS_NODISTURB	3

#define STATUS_BEGIN		STATUS_OFFLINE
#define STATUS_END		STATUS_NODISTURB

class user_status_resource : public http_resource
{
public:
	user_status_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_POST(const http_request&);

private:
	db_connection_pool& pool;
};

#endif
