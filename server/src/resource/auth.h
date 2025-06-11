#ifndef RESOURCE_AUTH_H
#define RESOURCE_AUTH_H

#include "db/conn_pool.h"
#include <thread>
#include <resource/base.h>

typedef std::string session_token;

class auth_resource : public base_resource
{
public:
	auth_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	session_token create_session(int user_id, pqxx::work& tx); // ensures that there are no duplicates

	/* CONFIG PARAMETERS */

	// in seconds
	size_t session_lifetime = 1800;
	size_t cleanup_period = 3600;
	
	unsigned min_username_length = 2;
	unsigned min_password_length = 8;
private:
	db_connection_pool& pool;

	std::thread session_time_thr;
	static void session_time_func(auth_resource& inst);
};

#endif
