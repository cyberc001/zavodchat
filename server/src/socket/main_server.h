#ifndef SOCKET_MAIN_SERVER_H
#define SOCKET_MAIN_SERVER_H

#include "socket/server.h"
#include <shared_mutex>
#include <functional>
#include "parallel_hashmap/phmap.h"

using main_server_recv_cb = std::function<void(int user_id, socket_event ev)>;

class socket_main_server: public socket_server
{
public:
	socket_main_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool);

	void send_to_server(int server_id, pqxx::work& tx, socket_event event);
	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event);
	void send_to_user(int user_id, pqxx::work& tx, socket_event event);
	// sends an event to everyone in the same servers as user
	void send_to_user_observers(int user_id, pqxx::work& tx, socket_event event);

private:
	void try_send_to_conn(int user_id, const std::string& data);

	phmap::parallel_flat_hash_map<int, std::shared_ptr<socket_connection>,
					phmap::priv::hash_default_hash<int>, phmap::priv::hash_default_eq<int>,
					phmap::priv::Allocator<std::pair<const int, std::shared_ptr<socket_connection>>>, 4,
					std::mutex> connections;
};

#endif
