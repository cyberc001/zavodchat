#ifndef RESOURCE_CLOCK_H
#define RESOURCE_CLOCK_H

#include <httpserver.hpp>
using namespace httpserver;

class clock_resource : public http_resource
{
public:
	clock_resource();

	std::shared_ptr<http_response> render_GET(const http_request&);
private:
};

#endif
