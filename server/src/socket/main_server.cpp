#include "socket/main_server.h"
#include <resource/user_status.h>

// DELETE
#include <iostream>

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
					r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(parse_token(msg)));
				} catch(pqxx::data_exception& e){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
					return;
				}
				if(!r.size()){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
					return;
				}
				conn.user_id = r[0]["user_id"].as<int>();

				// set status
				int status = STATUS_ONLINE;
				if(query.find("status") != query.end()){
					try{
						status = std::stoi(query["status"]);
					} catch(std::invalid_argument& e) {
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid status");
						return;
					}
					if(status < STATUS_BEGIN || status > STATUS_END){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid status");
						return;
					}
				}
				tx.exec("UPDATE users SET status = $1 WHERE user_id = $2", pqxx::params(status, conn.user_id));
				tx.commit();

				// add user to connections map
				std::unique_lock lock(connections_mutex);
				connections[conn.user_id] = conn.sock;
			} else if(msg->type == ix::WebSocketMessageType::Close){
				// set status to offline
				db_connection db_conn = pool.hold();
				pqxx::work tx{*db_conn};
				tx.exec("UPDATE users SET status = 0 WHERE user_id = $1", pqxx::params(conn.user_id));
				tx.commit();

				// remove user from connections map
				std::unique_lock lock(connections_mutex);
				connections.erase(conn.user_id);
			} else if(msg->type == ix::WebSocketMessageType::Message){
				for(auto it = recv_cbs.begin(); it != recv_cbs.end(); ++it){
					(*it)(conn.user_id, socket_event(msg->str));
				}
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

void socket_main_server::add_recv_cb(main_server_recv_cb cb)
{
	recv_cbs.push_back(cb);
}
