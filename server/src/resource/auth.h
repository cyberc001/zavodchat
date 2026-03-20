#ifndef RESOURCE_AUTH_H
#define RESOURCE_AUTH_H

#include "socket/main_server.h"
#include <thread>
#include <resource/base.h>

typedef std::string session_token;

class auth_resource : public base_resource
{
public:
	auth_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_POST(const http_request&);

	session_token create_session(int user_id, pqxx::work& tx); // ensures that there are no duplicates
	
private:
	std::thread session_time_thr;
	static void session_time_func(auth_resource& inst);
};
class register_resource : public base_resource
{
public:
	register_resource(webserver& ws, db_connection_pool& pool, const config& cfg,
				socket_main_server& sserv);

	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
private:
	socket_main_server& sserv;
};

#endif
