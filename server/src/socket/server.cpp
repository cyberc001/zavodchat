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

	srv.setOnClientMessageCallback([&](std::shared_ptr<ix::ConnectionState> _conn, ix::WebSocket& sock, const ix::WebSocketMessagePtr& msg){
		socket_connection& conn = dynamic_cast<socket_connection&>(*_conn);
		if(msg->type == ix::WebSocketMessageType::Open){
			auto query = parse_query(msg->openInfo.uri);
			db_connection db_conn = pool.hold();
			pqxx::work tx{*db_conn};
			pqxx::result r;
			try{
				r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(query["token"]));
			} catch(pqxx::data_exception& e){
				sock.close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
				return;
			}
			if(!r.size()){
				sock.close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
				return;
			}
			conn.user_id = r[0]["user_id"].as<int>();

			
		}
	});

	auto res = srv.listen();
	srv.start();
	std::cerr << "Listening for WSS on port " << port << "...\n";
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
