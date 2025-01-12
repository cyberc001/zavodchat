#include "socket/server.h"

#include <iostream>
#include <thread>

std::string socket_event::dump()
{
	nlohmann::json obj = {{"name", name}, {"data", data}};
	return obj.dump();
}


socket_server::socket_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool): srv{port, "0.0.0.0"}, pool{pool}
{
	ix::SocketTLSOptions tls_opt;
	tls_opt.certFile = https_cert;
	tls_opt.keyFile = https_key;
	tls_opt.caFile = "NONE";
	tls_opt.tls = true;
	srv.setTLSOptions(tls_opt);

	srv.setConnectionStateFactory([&](){
		return std::make_shared<socket_connection>();
	});

	srv.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		socket_connection& conn = dynamic_cast<socket_connection&>(*_conn);
		conn.sock = sock;

		sock.lock()->setOnMessageCallback([&](const ix::WebSocketMessagePtr& msg){
			if(msg->type == ix::WebSocketMessageType::Open){
				auto query = parse_query(msg->openInfo.uri);
				db_connection db_conn = pool.hold();
				pqxx::work tx{*db_conn};
				pqxx::result r;
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

void socket_server::listen()
{
	auto res = srv.listen();
	srv.start();
	std::cerr << "Listening for WSS on port " << srv.getPort() << "...\n";
	srv.wait();
}

std::unordered_map<std::string, std::string> socket_server::parse_query(std::string uri)
{
	std::unordered_map<std::string, std::string> res;
	size_t i = 0, ln = uri.length();
	for(; i < ln; ++i)
		if(uri[i] == '?')
			break;
	if(i == ln)
		return res;

	for(++i; i < ln; ++i){
		size_t begin = i;
		for(; i < ln; ++i)
			if(uri[i] == '&')
				break;
		if(i > begin){
			std::string query = uri.substr(begin, i - begin);
			size_t eq_i = query.find_first_of('=');
			if(eq_i != std::string::npos)
				res[query.substr(0, eq_i)] = query.substr(eq_i + 1);
		}
	}
	return res;
}

void socket_server::send_to_server(int server_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1", pqxx::params(server_id));

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
void socket_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();
	pqxx::result r = tx.exec("SELECT server_id FROM channels WHERE channel_id = $1", pqxx::params(channel_id));
	int server_id = r[0]["server_id"].as<int>();
	r = tx.exec("SELECT user_id FROM user_x_server WHERE server_id = $1", pqxx::params(server_id));

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
void socket_server::send_to_user(int user_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	std::shared_lock lock(connections_mutex);
	if(connections.find(user_id) != connections.end()){
		std::shared_ptr<ix::WebSocket> sock = connections[user_id].lock();
		if(sock)
			sock->send(dumped);
	}
}
