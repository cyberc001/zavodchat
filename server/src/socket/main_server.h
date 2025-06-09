#ifndef SOCKET_MAIN_SERVER_H
#define SOCKET_MAIN_SERVER_H

#include "socket/server.h"
#include <shared_mutex>
#include <functional>

using main_server_recv_cb = std::function<void(int user_id, socket_event ev)>;

class socket_main_server: public socket_server
{
public:
	socket_main_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	void send_to_server(int server_id, pqxx::work& tx, socket_event event);
	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event);
	void send_to_user(int user_id, pqxx::work& tx, socket_event event);

	void add_recv_cb(main_server_recv_cb cb);
private:
	std::shared_mutex connections_mutex;
	std::unordered_map<int, std::weak_ptr<ix::WebSocket>> connections;
	std::vector<main_server_recv_cb> recv_cbs;
};

#endif
