#include <resource/base.h>
#include <resource/utils.h>

#include <thread>
#include <chrono>

size_t base_resource::response_delay = 0;

base_resource::base_resource()
{
	disallow_all();
	set_allowing("OPTIONS", true);
}

std::shared_ptr<http_response> base_resource::render_OPTIONS(const http_request& req)
{
	return create_response::string(req, "", 200);
}

std::shared_ptr<http_response> base_resource::render_GET(const http_request&)
{
	if(response_delay > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(response_delay));
	return std::make_shared<string_response>();
}
std::shared_ptr<http_response> base_resource::render_POST(const http_request&)
{
	if(response_delay > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(response_delay));
	return std::make_shared<string_response>();
}
std::shared_ptr<http_response> base_resource::render_PUT(const http_request&)
{
	if(response_delay > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(response_delay));
	return std::make_shared<string_response>();
}
std::shared_ptr<http_response> base_resource::render_DELETE(const http_request&)
{
	if(response_delay > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(response_delay));
	return std::make_shared<string_response>();
}
