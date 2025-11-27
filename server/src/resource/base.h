#ifndef RESOURCE_BASE_H
#define RESOURCE_BASE_H

#include <httpserver.hpp>
using namespace httpserver;

class base_resource : public http_resource
{
public:
	base_resource();

	std::shared_ptr<http_response> render_OPTIONS(const http_request&);

	// artificial response delay
	static size_t response_delay;
	std::shared_ptr<http_response> render_GET(const http_request&);
	std::shared_ptr<http_response> render_POST(const http_request&);
	std::shared_ptr<http_response> render_PUT(const http_request&);
	std::shared_ptr<http_response> render_DELETE(const http_request&);
};

#endif
