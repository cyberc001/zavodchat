#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/server.h"
#include <shared_mutex>

class socket_vc_connection : public socket_connection
{
public:
	int channel_id;
};

class socket_vc_server: public socket_server
{
public:
	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
private:
	std::shared_mutex connections_mutex;
	// channel_id -> user_id -> socket
	std::unordered_map<int, std::unordered_map<int, std::weak_ptr<ix::WebSocket>>> connections;
};

#endif
