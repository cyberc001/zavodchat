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
}

void socket_server::listen(bool wait)
{
	auto res = srv.listen();
	srv.start();
	std::cerr << "Listening for WSS on port " << srv.getPort() << "...\n";
	if(wait)
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
