#ifndef RESOURCE_USER_H
#define RESOURCE_USER_H

#include <resource/base.h>

class user_resource : public base_resource
{
public:
	user_resource(webserver& ws, db_connection_pool& pool, const config& cfg);
	std::shared_ptr<http_response> render_GET(const http_request&);
};

class user_id_resource : public base_resource
{
public:
	user_id_resource(webserver& ws, db_connection_pool& pool, const config& cfg);
	std::shared_ptr<http_response> render_GET(const http_request&);

	std::shared_ptr<http_response> parse_id(const http_request&, pqxx::work&, int& user_id);
};

#endif
