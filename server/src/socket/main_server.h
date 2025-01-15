#ifndef SOCKET_MAIN_SERVER_H
#define SOCKET_MAIN_SERVER_H

#include "socket/server.h"

#include <shared_mutex>

class socket_main_server: public socket_server
{
public:
	socket_main_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	void send_to_server(int server_id, pqxx::work& tx, socket_event event);
	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event);
	void send_to_user(int user_id, pqxx::work& tx, socket_event event);
private:
	std::shared_mutex connections_mutex;
	std::unordered_map<int, std::weak_ptr<ix::WebSocket>> connections;
};

#endif
