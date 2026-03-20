#ifndef RESOURCE_PARAMS_H
#define RESOURCE_PARAMS_H

#include <resource/base.h>

class params_resource : public base_resource
{
public:
	params_resource(webserver& ws, db_connection_pool& pool, const config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);
};

#endif
