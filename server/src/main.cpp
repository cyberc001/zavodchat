#include <pqxx/pqxx>
#include <fstream>
#include <iostream>

#include "config.h"

#include "resource/auth.h"
#include "resource/server.h"
#include "resource/user.h"
#include "resource/server_users.h"
#include "resource/server_channels.h"
#include "resource/channel_messages.h"
#include "resource/server_invites.h"
#include "resource/server_bans.h"

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
	db_connection_pool pool{cfg.get_conn_str()};

	httpserver::webserver ws = httpserver::create_webserver(cfg.https_port)
								.use_ssl()
								.https_mem_key(cfg.https_key)
								.https_mem_cert(cfg.https_cert);
	socket_main_server sserv(cfg.https_key, cfg.https_cert, cfg.ws_port, pool);
	socket_vc_server vcserv(cfg.https_key, cfg.https_cert, cfg.ws_vc_port, pool);

	auth_resource auth(pool);
	auth.min_username_length = cfg.min_username_length;
	auth.min_password_length = cfg.min_password_length;
	auth.session_lifetime = cfg.session_lifetime;
	auth.cleanup_period = cfg.cleanup_period;
	ws.register_resource("/auth", &auth);

	server_resource server(pool);
	server.owned_per_user = cfg.servers_owned_per_user;
	ws.register_resource("/servers", &server);
	server_id_resource server_id(pool, sserv);
	ws.register_resource("/servers/{server_id}", &server_id);

	server_users_resource server_users(pool);
	server_users.max_get_count = cfg.max_get_count;
	ws.register_resource("/servers/{server_id}/users", &server_users);
	server_user_id_resource server_user_id(pool);
	ws.register_resource("/servers/{server_id}/users/{server_user_id}", &server_user_id);

	server_channel_resource server_channels(pool, sserv);
	server_channels.max_per_server = cfg.max_channels_per_server;
	ws.register_resource("/servers/{server_id}/channels", &server_channels);
	server_channel_id_resource server_channel_id(pool, sserv, vcserv);
	ws.register_resource("/servers/{server_id}/channels/{channel_id}", &server_channel_id);

	channel_messages_resource channel_messages(pool, sserv);
	channel_messages.max_get_count = cfg.max_get_count;
	ws.register_resource("/servers/{server_id}/channels/{channel_id}/messages", &channel_messages);
	channel_message_id_resource channel_message_id(pool, sserv);
	ws.register_resource("/servers/{server_id}/channels/{channel_id}/messages/{message_id}", &channel_message_id);

	server_invites_resource server_invites(pool);
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
	server_ban_id_resource server_ban_id(pool);
	ws.register_resource("/servers/{server_id}/bans/{server_ban_id}", &server_ban_id);

	user_id_resource user_id(pool);
	ws.register_resource("/users/{user_id}", &user_id);

	ws.start(false);
	std::cerr << "Listening for HTTPS on port " << cfg.https_port << "...\n";
	sserv.listen(false);
	vcserv.listen(true);
}
