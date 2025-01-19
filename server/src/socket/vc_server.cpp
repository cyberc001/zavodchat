#include "socket/vc_server.h"
#include "resource/server_channels.h"
#include "resource/utils.h"

#include <iostream>

socket_vc_server::socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv): socket_server(https_key, https_cert, port, pool), sserv{sserv}
{
	srv.setConnectionStateFactory([&](){
		return std::make_shared<socket_vc_connection>();
	});

	srv.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		socket_vc_connection& conn = dynamic_cast<socket_vc_connection&>(*_conn);
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

				// check voice channel
				try{
					conn.channel_id = std::stol(query["channel"]);
				} catch(std::invalid_argument& e){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Couldn't parse channel ID, got '" + query["channel"] + "'");
					return;
				}
				r = tx.exec("SELECT type, server_id, channel_id FROM channels WHERE channel_id = $1", pqxx::params(conn.channel_id));
				if(!r.size()){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it doesn't exist");
					return;
				}
				conn.server_id = r[0]["server_id"].as<int>();
				int ch_type = r[0]["type"].as<int>();
				if(resource_utils::check_server_member(conn.user_id, conn.server_id, tx)){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it doesn't exist");
					return;
				}
				if(ch_type != CHANNEL_VOICE){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Isn't a voice channel");
					return;
				}

				std::unique_lock lock(connections_mutex);
				if(connections[conn.channel_id].find(conn.user_id) != connections[conn.channel_id].end()){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User is already connected");
					return;
				}
				connections[conn.channel_id][conn.user_id] = conn.sock;

				socket_event ev;
				resource_utils::json_set_ids(ev.data, conn.server_id, conn.channel_id);
				ev.data["id"] = conn.user_id;
				ev.name = "user_joined_vc";
				sserv.send_to_channel(conn.channel_id, tx, ev);

			} else if(msg->type == ix::WebSocketMessageType::Close){
				if(msg->closeInfo.reason != "User is already connected"){
					std::unique_lock lock(connections_mutex);
					connections[conn.channel_id].erase(conn.user_id);

					db_connection db_conn = pool.hold();
					pqxx::work tx{*db_conn};

					socket_event ev;
					resource_utils::json_set_ids(ev.data, conn.server_id, conn.channel_id);
					ev.data["id"] = conn.user_id;
					ev.name = "user_left_vc";
					sserv.send_to_channel(conn.channel_id, tx, ev);
				}
			}
		});
	});
}

void socket_vc_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	std::shared_lock lock(connections_mutex);
	const auto& channel_users = connections[channel_id];
	for(auto it = channel_users.begin(); it != channel_users.end(); ++it){
		std::shared_ptr<ix::WebSocket> sock = it->second.lock();
		if(sock)
			sock->send(dumped);
	}
}

void socket_vc_server::get_channel_users(int channel_id, std::vector<int>& users)
{
	std::shared_lock lock(connections_mutex);
	const auto& channel_users = connections[channel_id];
	for(auto it = channel_users.begin(); it != channel_users.end(); ++it)
		users.push_back(it->first);
}
