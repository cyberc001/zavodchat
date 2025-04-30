#ifndef RESOURCE_USER_H
#define RESOURCE_USER_H

#include "db/conn_pool.h"
#include <resource/base.h>

class user_id_resource : public base_resource
{
public:
	user_id_resource(db_connection_pool& pool);
	std::shared_ptr<http_response> render_GET(const http_request&);

	std::shared_ptr<http_response> parse_id(const http_request&, pqxx::work&, int& user_id);
private:
	db_connection_pool& pool;
};

#endif
