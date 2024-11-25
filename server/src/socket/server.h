#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <unordered_map>

#include <nlohmann/json.hpp>
#include <ixwebsocket/IXWebSocketServer.h>

#include "db/conn_pool.h"
#include "socket/thread_queue.h"

class socket_event
{
	std::string name;
	nlohmann::json data;

	std::string dump();
};

class socket_server
{
public:
	socket_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	void send_to_server(int server_id, socket_event event);
	void send_to_channel(int channel_id, socket_event event);
private:
	ix::WebSocketServer srv;
	db_connection_pool& pool;

	std::unordered_multimap<int, std::weak_ptr<ix::WebSocket>> server_users;
	std::unordered_multimap<int, std::weak_ptr<ix::WebSocket>> channel_users;
};

#endif
