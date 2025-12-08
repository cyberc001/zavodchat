#ifndef USER_STATUS_H
#define USER_STATUS_H

#include "db/conn_pool.h"
#include "socket/main_server.h"
#include <resource/base.h>

#define STATUS_OFFLINE		0
#define STATUS_ONLINE		1
#define STATUS_AWAY		2
#define STATUS_NODISTURB	3

#define STATUS_BEGIN		STATUS_OFFLINE
#define STATUS_END		STATUS_NODISTURB

class user_status_resource : public base_resource
{
public:
	user_status_resource(db_connection_pool& pool, socket_main_server& sserv);

	std::shared_ptr<http_response> render_PUT(const http_request&);

private:
	db_connection_pool& pool;
	socket_main_server& sserv;
};

#endif
