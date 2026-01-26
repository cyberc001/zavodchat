#include <fstream>
#include <iostream>

#include "config.h"

#include "resource/auth.h"
#include "resource/server.h"
#include "resource/user.h"
#include "resource/file.h"
#include "resource/file_utils.h"
#include "resource/server_users.h"
#include "resource/channel.h"
#include "resource/message.h"
#include "resource/server_invites.h"
#include "resource/server_bans.h"
#include "resource/role.h"
#include "resource/user_status.h"
#include "resource/params.h"

#include "db/init.h"

#include "socket/main_server.h"
#include "socket/vc_server.h"

int main()
{
	std::ifstream fd{"config.json"};
	config cfg;
	try {
		cfg = config{fd};
	} catch(std::logic_error& e) {
		std::cerr << "Error parsing config: " << e.what() << "\n";
		return -1;
	}

	db_connect(cfg.get_conn_str());
	db_init(cfg.get_conn_str());
	if(cfg.create_test_db)
		db_create_test(cfg.get_conn_str());
	db_connection_pool pool{cfg.get_conn_str()};

	file_utils::user_avatar_storage_path = cfg.user_avatar_path;
	file_utils::server_avatar_storage_path = cfg.server_avatar_path;
	file_utils::file_storage_path = cfg.file_storage_path;
	file_utils::file_storage_size = cfg.file_storage_size;


	base_resource::response_delay = cfg.response_delay;

	httpserver::webserver ws = httpserver::create_webserver(cfg.https_port)
								.use_ssl()
								.https_mem_key(cfg.https_key)
								.https_mem_cert(cfg.https_cert);
	socket_main_server sserv(cfg.https_key, cfg.https_cert, cfg.ws_port, pool);
	socket_vc_server vcserv(cfg.https_key, cfg.https_cert, cfg.ws_vc_port,
					pool, sserv, cfg.rtc_addr, cfg.rtc_port);
	vcserv.max_video_bitrate = cfg.max_video_bitrate;

	create_response::set_origins(cfg.origins);

	auth_resource auth(pool);
	auth.session_lifetime = cfg.session_lifetime;
	auth.cleanup_period = cfg.cleanup_period;
	auth.sessions_per_user = cfg.sessions_per_user;
	ws.register_resource("/auth", &auth);
	register_resource _register(pool, sserv);
	_register.min_username_length = cfg.min_username_length;
	_register.min_password_length = cfg.min_password_length;
	ws.register_resource("/register", &_register);

	server_resource server(pool);
	server.owned_per_user = cfg.servers_owned_per_user;
	ws.register_resource("/servers", &server);
	server_id_resource server_id(pool, sserv);
	server_id.owned_per_user = cfg.servers_owned_per_user;
	ws.register_resource("/servers/{server_id}", &server_id);

	server_users_resource server_users(pool);
	server_users.max_get_count = cfg.max_get_count;
	ws.register_resource("/servers/{server_id}/users", &server_users);
	server_user_id_resource server_user_id(pool, sserv);
	ws.register_resource("/servers/{server_id}/users/{server_user_id}", &server_user_id);
	server_user_role_id_resource server_user_role_id(pool, sserv);
	ws.register_resource("/servers/{server_id}/users/{server_user_id}/roles/{server_role_id}", &server_user_role_id);

	server_channel_resource server_channels(pool, sserv, vcserv);
	server_channels.max_per_server = cfg.max_channels_per_server;
	ws.register_resource("/servers/{server_id}/channels", &server_channels);
	channel_resource channel(pool, sserv, vcserv);
	ws.register_resource("/channels/{channel_id}", &channel);

	channel_messages_resource channel_messages(pool, sserv);
	channel_messages.max_get_count = cfg.max_get_count;
	ws.register_resource("/channels/{channel_id}/messages", &channel_messages);
	message_resource message(pool, sserv);
	ws.register_resource("/messages/{message_id}", &message);

	server_invites_resource server_invites(pool, sserv);
	server_invites.cleanup_period = cfg.cleanup_period;
	ws.register_resource("/server_invites/{invite_id}", &server_invites);
	server_id_invites_resource server_id_invites(pool);
	ws.register_resource("/servers/{server_id}/invites", &server_id_invites);
	server_invite_id_resource server_invite_id(pool);
	ws.register_resource("/servers/{server_id}/invites/{invite_id}", &server_invite_id);

	server_bans_resource server_bans(pool);
	server_bans.max_get_count = cfg.max_get_count;
	server_bans.cleanup_period = cfg.cleanup_period;
	ws.register_resource("/servers/{server_id}/bans", &server_bans);
	server_ban_id_resource server_ban_id(pool, sserv);
	ws.register_resource("/servers/{server_id}/bans/{server_ban_id}", &server_ban_id);

	user_id_resource user_id(pool);
	ws.register_resource("/users/{user_id}", &user_id);

	user_status_resource user_status(pool, sserv);
	ws.register_resource("/user_status", &user_status);

	server_roles_resource server_roles(pool, sserv);
	server_roles.max_per_server = cfg.max_roles_per_server;
	ws.register_resource("/servers/{server_id}/roles", &server_roles);

	file_resource user_avatars(cfg.user_avatar_path);
	ws.register_resource("/files/avatar/user/{fname}", &user_avatars);
	file_resource server_avatars(cfg.server_avatar_path);
	ws.register_resource("/files/avatar/server/{fname}", &server_avatars);

	server_file_put_resource server_file_put(pool, cfg.file_storage_path);
	ws.register_resource("/files/upload/", &server_file_put);
	server_file_manage_resource server_file_manage(pool, cfg.file_storage_path);
	ws.register_resource("/files/upload/{fname}", &server_file_manage);
	server_user_file_resource server_user_file(pool, cfg.file_storage_path);
	ws.register_resource("/files/upload/{user_id}/{fname}", &server_user_file);

	params_resource params(cfg);
	ws.register_resource("/params", &params);


	ws.start(false);
	std::cerr << "Listening for HTTPS on port " << cfg.https_port << "...\n";
	sserv.listen(false);
	vcserv.listen(true);
}
