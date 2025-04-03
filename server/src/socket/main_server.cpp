#include "socket/main_server.h"

socket_main_server::socket_main_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool): socket_server(https_key, https_cert, port, pool)
{
	srv.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		socket_connection& conn = dynamic_cast<socket_connection&>(*_conn);
		conn.sock = sock;

		sock.lock()->setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg){
			if(msg->type == ix::WebSocketMessageType::Open){
				auto query = parse_query(msg->openInfo.uri);
				db_connection db_conn = pool.hold();
				pqxx::work tx{*db_conn};
				pqxx::result r;

				// check auth token
				try{
					r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(query["token"]));
				} catch(pqxx::data_exception& e){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
					return;
				}
				if(!r.size()){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
					return;
				}
				conn.user_id = r[0]["user_id"].as<int>();

				std::unique_lock lock(connections_mutex);
				connections[conn.user_id] = conn.sock;
			} else if(msg->type == ix::WebSocketMessageType::Close){
				std::unique_lock lock(connections_mutex);
				connections.erase(conn.user_id);
			}
		});
	});
}

void socket_main_server::send_to_server(int server_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1 GROUP BY user_id", pqxx::params(server_id));

	std::shared_lock lock(connections_mutex);
	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		if(connections.find(user_id) != connections.end()){
			std::shared_ptr<ix::WebSocket> sock = connections[user_id].lock();
			if(sock)
				sock->send(dumped);
		}
	}
}
void socket_main_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT server_id FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	int server_id = r[0]["server_id"].as<int>();
	r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1 GROUP BY user_id", pqxx::params(server_id));

	std::shared_lock lock(connections_mutex);
	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		if(connections.find(user_id) != connections.end()){
			std::shared_ptr<ix::WebSocket> sock = connections[user_id].lock();
			if(sock)
				sock->send(dumped);
		}
	}
}
void socket_main_server::send_to_user(int user_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	std::shared_lock lock(connections_mutex);
	if(connections.find(user_id) != connections.end()){
		std::shared_ptr<ix::WebSocket> sock = connections[user_id].lock();
		if(sock)
			sock->send(dumped);
	}
}
