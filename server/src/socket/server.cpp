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

	srv.setOnClientMessageCallback([&pool](std::shared_ptr<ix::ConnectionState> conn, ix::WebSocket& sock, const ix::WebSocketMessagePtr& msg){
		if(msg->type == ix::WebSocketMessageType::Open){
			// parse token
			std::cerr << conn->getRemoteIp();

			// insert user socket into relevant maps
			db_connection conn = pool.hold();
			pqxx::work tx{*conn};

			//pqxx::result r = tx.exec("SELECT server_id, name, avatar FROM user_x_server NATURAL JOIN servers WHERE user_id = $1", pqxx::params(user_id));
		}
	});

	auto res = srv.listen();
	srv.start();
	srv.wait();
}

void socket_server::send_to_server(int server_id, socket_event event)
{
	/*std::string dumped = event.dump();
	for(auto it = server_users.find(server_id); it != server_users.end(); ++it)
		it->second.sendUtf8Text(dumped);*/
}
void socket_server::send_to_channel(int channel_id, socket_event event)
{
	/*std::string dumped = event.dump();
	for(auto it = channel_users.find(channel_id); it != channel_users.end(); ++it)
		it->second.sendUtf8Text(dumped);*/
}
