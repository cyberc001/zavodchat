#include "resource/file.h"
#include "resource/utils.h"

file_resource::file_resource(std::filesystem::path storage_path): storage_path{storage_path}
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> file_resource::render_GET(const http_request& req)
{
	std::string_view fname = req.get_arg("fname");
	auto res = create_response::file(storage_path / fname);
	return res;
}
