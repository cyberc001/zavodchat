#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/main_server.h"
#include "rtc/rtc.hpp"
#include <shared_mutex>
#include <stack>
#include <random>

class socket_vc_connection : public socket_connection
{
public:
	int server_id = -1;
	int channel_id = -1;

	std::shared_ptr<rtc::PeerConnection> rtc_conn;

	// tracks are re-used, and vector is never shrunk, otherwise tracks are gonna be rejected until PeerConnection is re-opened.
	std::vector<std::shared_ptr<rtc::Track>> tracks; // 0 - voice track receiver
	
	size_t add_audio_track(rtc::SSRC ssrc, int user_id);
	void remove_audio_track(int user_id);
	std::stack<size_t> unused_audio_tracks;
	std::unordered_map<int, size_t> user_to_audio_track; // user id to track index
	
	size_t add_recv_video_track(rtc::SSRC ssrc);
	size_t add_video_track(rtc::SSRC ssrc, int user_id);
	void remove_video_track(int user_id);
	std::stack<size_t> unused_video_tracks;
	std::unordered_map<int, size_t> user_to_video_track; // -1 - this user (recv track) - optional
	
	bool recv_video_track_requested_bitrate = false;
	unsigned recv_video_track_bitrate;
	std::set<size_t> users_needing_keyframe; // user IDs that, after appearing in connections list, should be generated keyframes for
};
class socket_vc_channel
{
public:
	// these functions use connections_mutex:
	void add_user(int user_id, std::shared_ptr<socket_vc_connection> conn);
	void remove_user(int user_id);
	bool has_user(int user_id);

	// connections_mutex 
	std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator connections_begin() const;
	std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator connections_end() const;

	std::shared_mutex connections_mutex;
private:
	std::unordered_map<int, std::weak_ptr<socket_vc_connection>> connections;
};

#define RTC_PAYLOAD_TYPE_VOICE 96
#define RTC_PAYLOAD_TYPE_VIDEO 97

class socket_vc_server: public socket_server
{
public:
	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				std::string rtc_addr, int rtc_port);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
	
	void get_channel_users(int channel_id, std::vector<int>& users); // get users connected to voice channel

	std::unordered_map<int, socket_vc_channel> channels;

	/* CONFIG PARAMETERS */
	unsigned max_video_bitrate = 10240000;
private:
	db_connection_pool& pool;
	socket_main_server& sserv;

	std::string rtc_addr;
	int rtc_port;
	std::string rtc_cert, rtc_key;

	// for generating SSRC
	std::random_device rndev;
	std::mt19937 rneng;
	std::uniform_int_distribution<std::mt19937::result_type> rndist;

	std::shared_mutex channels_mutex;
};

#endif
