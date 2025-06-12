#ifndef RESOURCE_PARAMS_H
#define RESOURCE_PARAMS_H

#include <resource/base.h>
#include "config.h"

class params_resource : public base_resource
{
public:
	params_resource(config& cfg);

	std::shared_ptr<http_response> render_GET(const http_request&);

private:
	config& cfg;
};

#endif
