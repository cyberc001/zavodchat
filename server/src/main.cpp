#include <fstream>
#include <iostream>

#include "config.h"

#include "resource/auth.h"
#include "resource/blocked_user.h"
#include "resource/server.h"
#include "resource/user.h"
#include "resource/dm.h"
#include "resource/file.h"
#include "resource/file_utils.h"
#include "resource/friend.h"
#include "resource/server_users.h"
#include "resource/channel.h"
#include "resource/message.h"
#include "resource/server_invites.h"
#include "resource/server_bans.h"
#include "resource/role.h"
#include "resource/notifications.h"
#include "resource/user_status.h"
#include "resource/params.h"
#include "resource/preferences.h"

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

	file_utils::file_storage_path = cfg.file_storage_path;
	file_utils::file_storage_size = cfg.file_storage_size;

	base_resource::response_delay = cfg.response_delay;

	httpserver::webserver ws = httpserver::create_webserver(cfg.https_port)
								.use_ssl()
								.https_mem_key(cfg.https_key)
								.https_mem_cert(cfg.https_cert);
	socket_main_server sserv(cfg.https_key, cfg.https_cert, cfg.ws_port, pool);
	socket_vc_server vcserv(cfg, pool, sserv);

	create_response::set_origins(cfg.origins);

	auth_resource auth(ws, pool, cfg);
	register_resource _register(ws, pool, cfg, sserv);

	friends_resource friends(ws, pool, cfg, vcserv);
	friend_requests_resource friend_requests(ws, pool, cfg);
	friends_id_resource friends_id(ws, pool, cfg, sserv);

	blocked_users_resource blocked_users(ws, pool, cfg);
	blocked_users_id_resource blocked_users_id(ws, pool, cfg, sserv);

	server_resource server(ws, pool, cfg);
	server_id_resource server_id(ws, pool, cfg, sserv);

	server_users_resource server_users(ws, pool, cfg);
	server_user_id_resource server_user_id(ws, pool, cfg, sserv);
	server_user_role_id_resource server_user_role_id(ws, pool, cfg, sserv);

	server_channel_resource server_channels(ws, pool, cfg, sserv, vcserv);
	channel_resource channel(ws, pool, cfg, sserv, vcserv);
	channel_user_id_resource channel_user_id(ws, pool, cfg, vcserv);
	channel_roles_resource channel_roles(ws, pool, cfg, sserv);

	dm_resource dm(ws, pool, cfg, vcserv);
	dm_id_resource dm_id(ws, pool, cfg);

	channel_messages_resource channel_messages(ws, pool, cfg, sserv);
	channel_messages_search_resource channel_messages_search(ws, pool, cfg, sserv);
	message_resource message(ws, pool, cfg, sserv);
	
	server_invites_resource server_invites(ws, pool, cfg, sserv);
	server_id_invites_resource server_id_invites(ws, pool, cfg);
	server_invite_id_resource server_invite_id(ws, pool, cfg);

	server_bans_resource server_bans(ws, pool, cfg);
	server_ban_id_resource server_ban_id(ws, pool, cfg, sserv);

	notifications_resource notifications(ws, pool, cfg);
	notification_channel_resource notification_channel(ws, pool, cfg);

	user_resource user(ws, pool, cfg);
	user_id_resource user_id(ws, pool, cfg);
	user_status_resource user_status(ws, pool, cfg, sserv);

	server_roles_resource server_roles(ws, pool, cfg, sserv);

	file_resource user_avatars(ws, "/files/avatar/user/{fname}", pool, cfg, cfg.user_avatar_path);
	file_resource server_avatars(ws, "/files/avatar/server/{fname}", pool, cfg, cfg.server_avatar_path);

	server_file_put_resource server_file_put(ws, pool, cfg);
	server_file_manage_resource server_file_manage(ws, pool, cfg);
	server_user_file_resource server_user_file(ws, pool, cfg);

	preferences_resource preferences(ws, pool, cfg);

	params_resource params(ws, pool, cfg);

	ws.start(false);
	std::cerr << "Listening for HTTPS on port " << cfg.https_port << "...\n";
	sserv.listen(false);
	vcserv.listen(true);
}
