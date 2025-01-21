#include "clock.h"
#include <chrono>
#include "resource/utils.h"

clock_resource::clock_resource()
{
	disallow_all();
	set_allowing("GET", true);
}

std::shared_ptr<http_response> clock_resource::render_GET(const http_request& req)
{
	std::string_view arg_client_ts = req.get_arg("ts");
	if(!arg_client_ts.size())
		return create_response::string("Empty timestamp", 400);
	long long num_client_ts;
	try{
		num_client_ts = strtoll(std::string(arg_client_ts).c_str(), NULL, 10);
	} catch(std::invalid_argument& e){
		return create_response::string("Couldn't parse timestamp '" + std::string(arg_client_ts) + "'", 400);
	}

	std::chrono::microseconds client_ts{num_client_ts};

	const auto server_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());

	return create_response::string(std::to_string(server_ts.count()) + ";" + std::to_string(server_ts.count() - client_ts.count()), 200);
}
