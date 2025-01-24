#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/main_server.h"
#include "rtc/rtc.hpp"
#include <shared_mutex>
#include <random>

class socket_vc_connection : public socket_connection
{
public:
	int server_id = -1;
	int channel_id = -1;

	std::shared_ptr<rtc::PeerConnection> rtc_conn;
	std::shared_ptr<rtc::Track> track_voice;
};

#define RTC_PAYLOAD_TYPE_VOICE 97

class socket_vc_server: public socket_server
{
public:
	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
	
	void get_channel_users(int channel_id, std::vector<int>& users); // get users connected to voice channel
private:
	socket_main_server& sserv;

	// for generating SSRC
	std::random_device rndev;
	std::mt19937 rneng;
	std::uniform_int_distribution<std::mt19937::result_type> rndist;

	std::shared_mutex connections_mutex;
	// channel_id -> user_id -> socket
	std::unordered_map<int, std::unordered_map<int, std::weak_ptr<ix::WebSocket>>> connections;
};

#endif
