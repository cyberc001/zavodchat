#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <unordered_map>
#include <shared_mutex>

#include <nlohmann/json.hpp>
#include <ixwebsocket/IXWebSocketServer.h>

#include "db/conn_pool.h"
#include "socket/thread_queue.h"

struct socket_event
{
	std::string name;
	nlohmann::json data;

	std::string dump();
};

class socket_connection : public ix::ConnectionState
{
public:
	int user_id = -1;
	std::weak_ptr<ix::WebSocket> sock;
};

class socket_server
{
public:
	socket_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	static std::unordered_map<std::string, std::string> parse_query(std::string uri);

	void send_to_server(int server_id, pqxx::work& tx, socket_event event);
	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event);
private:
	ix::WebSocketServer srv;
	db_connection_pool& pool;

	std::shared_mutex connections_mutex;
	std::unordered_map<int, std::weak_ptr<ix::WebSocket>> connections;
};

#endif
