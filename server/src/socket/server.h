#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <unordered_map>

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
	void listen();

	static std::unordered_map<std::string, std::string> parse_query(std::string uri);
protected:
	ix::WebSocketServer srv;
	db_connection_pool& pool;
};

#endif
