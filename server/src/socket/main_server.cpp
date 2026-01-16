#include "socket/main_server.h"
#include <resource/user_status.h>
#include <resource/utils.h>

#include <iostream>

socket_main_server::socket_main_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool): socket_server(https_key, https_cert, port, pool)
{
	srv.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		std::shared_ptr<socket_connection> conn = std::dynamic_pointer_cast<socket_connection>(_conn);
		conn->sock = sock;

		sock.lock()->setOnMessageCallback([this, conn, &pool](const ix::WebSocketMessagePtr& msg){
			if(msg->type == ix::WebSocketMessageType::Open){
				auto query = parse_query(msg->openInfo.uri);
				db_connection db_conn = pool.hold();
				pqxx::work tx{*db_conn};
				pqxx::result r;

				// check auth token
				try{
					r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(parse_token(msg)));
				} catch(pqxx::data_exception& e){
					conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
					return;
				}
				if(!r.size()){
					conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
					return;
				}
				conn->user_id = r[0]["user_id"].as<int>();

				// set status
				int status = STATUS_ONLINE;
				if(query.find("status") != query.end()){
					try{
						status = std::stoi(query["status"]);
					} catch(std::invalid_argument& e) {
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid status");
						return;
					}
					if(status < STATUS_BEGIN || status > STATUS_END){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid status");
						return;
					}
				}

				connections[conn->user_id] = conn;

				socket_event ev;
				ev.data["id"] = conn->user_id;
				ev.data["status"] = status;
				ev.name = "user_changed";
				send_to_user_observers(conn->user_id, tx, ev);

				tx.exec("UPDATE users SET status = $1 WHERE user_id = $2", pqxx::params(status, conn->user_id));
				tx.commit();
			} else if(msg->type == ix::WebSocketMessageType::Close){
				// set status to offline
				db_connection db_conn = pool.hold();
				pqxx::work tx{*db_conn};

				connections.erase(conn->user_id);

				socket_event ev;
				ev.data["id"] = conn->user_id;
				ev.data["status"] = STATUS_OFFLINE;
				ev.name = "user_changed";
				send_to_user_observers(conn->user_id, tx, ev);

				tx.exec("UPDATE users SET status = 0 WHERE user_id = $1", pqxx::params(conn->user_id));
				tx.commit();
			}
		});
	});
}


void socket_main_server::try_send_to_conn(int user_id, const std::string& data)
{
	connections.if_contains(user_id, [&data](std::pair<int, std::shared_ptr<socket_connection>> p){
		p.second->send(data);
	});
}

void socket_main_server::send_to_server(int server_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1 GROUP BY user_id", pqxx::params(server_id));

	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		try_send_to_conn(user_id, dumped);
	}
}
void socket_main_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT server_id FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	int server_id = r[0]["server_id"].as<int>();
	r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1 GROUP BY user_id", pqxx::params(server_id));

	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		try_send_to_conn(user_id, dumped);
	}
}
void socket_main_server::send_to_user(int user_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	try_send_to_conn(user_id, dumped);
}
void socket_main_server::send_to_user_observers(int user_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT DISTINCT ON(user_id) user_id, server_id FROM user_x_server WHERE server_id IN (SELECT server_id FROM user_x_server WHERE user_id = $1)", pqxx::params(user_id));

	for(size_t i = 0; i < r.size(); ++i){
		int user_id = r[i]["user_id"].as<int>();
		try_send_to_conn(user_id, dumped);
	}
}
