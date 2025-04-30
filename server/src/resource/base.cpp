#include <resource/base.h>
#include <resource/utils.h>

base_resource::base_resource()
{
	disallow_all();
	set_allowing("OPTIONS", true);
}

std::shared_ptr<http_response> base_resource::render_OPTIONS(const http_request&)
{
	return create_response::string("", 200);
}
