#ifndef RESOURCE_PREFERENCES_H
#define RESOURCE_PREFERENCES_H

#include <resource/base.h>
#include <nlohmann/json.hpp>

class preferences_resource : public base_resource
{
public:
	preferences_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);

private:
	std::string json_value_string(const nlohmann::json& j);
};

#endif
