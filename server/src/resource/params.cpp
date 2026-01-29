#include "resource/params.h"
#include "resource/utils.h"

#include <nlohmann/json.hpp>

params_resource::params_resource(config& cfg): cfg(cfg)
{
	set_allowing("GET", true);
}

std::shared_ptr<http_response> params_resource::render_GET(const http_request& req)
{
	nlohmann::json obj = {
		{"ws_port", cfg.ws_port},
		{"ws_vc_port", cfg.ws_vc_port},
		{"min_username_length", cfg.min_username_length},
		{"min_password_length", cfg.min_password_length},
		{"total_file_storage_size", cfg.file_storage_size},
		{"max_get_count", cfg.max_get_count},
		{"max_user_preference_keys", cfg.max_user_preference_keys},
		{"servers_owned_per_user", cfg.servers_owned_per_user},
		{"max_channels_per_server", cfg.max_channels_per_server},
		{"max_roles_per_server", cfg.max_roles_per_server},
		{"max_video_bitrate", cfg.max_video_bitrate}
	};
	return create_response::string(req, obj.dump(), 200);
}
