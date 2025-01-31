#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/main_server.h"
#include "rtc/rtc.hpp"
#include <gst/gst.h>
#include <shared_mutex>
#include <random>

class socket_vc_connection : public socket_connection
{
public:
	void close(GstElement* pipeline); // to be called from socket_vc_channel::remove_user()

	int server_id = -1;
	int channel_id = -1;

	std::shared_ptr<rtc::PeerConnection> rtc_conn;
	std::shared_ptr<rtc::Track> track_voice;

	GstElement* appsrc = nullptr;
	GstElement* opuspay = nullptr;
	GstPad* muxer_sink = nullptr;
};
class socket_vc_channel
{
public:
	void add_user(int user_id, std::shared_ptr<socket_vc_connection> conn);
	void remove_user(int user_id);
	bool has_user(int user_id) const;

	std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator connections_begin() const;
	std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator connections_end() const;

//TODO uncomment
//private:
	std::unordered_map<int, std::weak_ptr<socket_vc_connection>> connections;
	GstElement* pipeline = nullptr;
	GstElement* muxer = nullptr;
	GstElement* appsink = nullptr;
};

#define RTC_PAYLOAD_TYPE_VOICE 96

class socket_vc_server: public socket_server
{
public:
	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				int rtc_port);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
	
	void get_channel_users(int channel_id, std::vector<int>& users); // get users connected to voice channel
private:
	db_connection_pool& pool;
	socket_main_server& sserv;
	std::thread gst_thr;
	int rtc_port;
	std::string rtc_cert, rtc_key;

	// for generating SSRC
	std::random_device rndev;
	std::mt19937 rneng;
	std::uniform_int_distribution<std::mt19937::result_type> rndist;

	std::shared_mutex channels_mutex;
	std::unordered_map<int, socket_vc_channel> channels;
};

#endif
