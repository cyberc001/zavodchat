#ifndef RESOURCE_PREFERENCES_H
#define RESOURCE_PREFERENCES_H

#include "db/conn_pool.h"
#include <resource/base.h>
#include <nlohmann/json.hpp>

class preferences_resource : public base_resource
{
public:
	preferences_resource(db_connection_pool& pool);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

	unsigned keys_per_user = 128;
private:
	db_connection_pool& pool;

	std::string json_value_string(const nlohmann::json& j);
};

#endif
