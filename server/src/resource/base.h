#ifndef RESOURCE_BASE_H
#define RESOURCE_BASE_H

#include <httpserver.hpp>
using namespace httpserver;

class base_resource : public http_resource
{
public:
	base_resource();

	std::shared_ptr<http_response> render_OPTIONS(const http_request&);
};

#endif
