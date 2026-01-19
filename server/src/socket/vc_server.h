#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/main_server.h"
#include "thread_pool.h"
#include "rtc/rtc.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <memory>
#include <queue>
#include <random>

#define RTC_PAYLOAD_TYPE_VOICE 96
#define RTC_PAYLOAD_TYPE_VIDEO 97

enum vc_state
{
	NO, SELF, BY_ADMIN
};


class socket_vc_connection : public socket_connection
{
public:
	int server_id = -1, channel_id = -1;
	std::shared_ptr<rtc::PeerConnection> rtc_conn;


	void init_rtc(std::string rtc_addr, int rtc_port, std::string rtc_cert, std::string rtc_key);

	void add_audio_track(rtc::SSRC, int user_id);
	void remove_audio_track(int user_id);
	std::shared_ptr<rtc::Track> get_audio_track(int user_id);

	void add_video_track(rtc::SSRC, int user_id, int bitrate);
	void remove_video_track(int user_id);
	std::shared_ptr<rtc::Track> get_video_track(int user_id);

	void add_recv_video_track(int bitrate);
	void remove_recv_video_track();
	// Returns nullptr if recv track does not exist or is closed
	std::shared_ptr<rtc::Track> get_recv_video_track();
	void set_recv_video_bitrate(int bitrate);

	// Queues a keyframe request for next frame outgoing to this user
	void request_keyframe(int user_id);
	// Erases user_id from requested keyframes
	bool is_keyframe_requested(int user_id);

	rtc::SSRC get_ssrc(std::shared_ptr<rtc::Track>);
	std::mutex& get_mutex();

	vc_state mute = vc_state::NO, deaf = vc_state::NO;

private:
	void send_offer(std::string rtc_addr);

	std::mutex mut;

	std::string get_new_mid();
	size_t last_mid = 0;
	void change_track_desc(std::shared_ptr<rtc::Track>, rtc::SSRC, int user_id = -1);

	// key: user_id; key == -1 -> recv track
	std::unordered_map<int, std::shared_ptr<rtc::Track>> audio_tracks;
	std::unordered_map<int, std::shared_ptr<rtc::Track>> video_tracks;
	std::queue<std::shared_ptr<rtc::Track>> removed_audio_tracks;
	std::queue<std::shared_ptr<rtc::Track>> removed_video_tracks;

	std::unordered_set<int> requested_keyframes; // key: user_id
	
	// This track shouldnt be removed
	void add_recv_audio_track();

	int video_recv_bitrate = -1;
	bool video_recv_is_open = false;
};

class socket_vc_channel
{
public:
	void add_user(std::shared_ptr<socket_vc_connection>, thread_pool& thr_pool);
	void remove_user(std::shared_ptr<socket_vc_connection>);
	std::shared_ptr<socket_vc_connection> get_user(int user_id);

	void enable_user_video(std::shared_ptr<socket_vc_connection>, thread_pool& thr_pool);
	void disable_user_video(std::shared_ptr<socket_vc_connection>);

	void for_each(std::function<void (int, std::shared_ptr<socket_vc_connection>)>);
private:
	std::mutex mut;
	std::unordered_map<int, std::shared_ptr<socket_vc_connection>> users;
};

class socket_vc_server : public socket_server
{
public:
	static thread_local std::mt19937 rng;

	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				std::string rtc_addr, int rtc_port);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
	
	nlohmann::json get_channel_users(int channel_id); // get users connected to voice channel


	int max_video_bitrate = 10240000;
private:
	bool parse_vc_state(std::unordered_map<std::string, std::string>& query, std::string arg_name, vc_state& out);
	bool check_vc_state(unsigned state);

	phmap::parallel_flat_hash_map<int, std::shared_ptr<socket_vc_channel>,
					phmap::priv::hash_default_hash<int>, phmap::priv::hash_default_eq<int>,
					phmap::priv::Allocator<std::pair<const int, std::shared_ptr<socket_vc_connection>>>, 4,
					std::mutex> channels;
	phmap::parallel_flat_hash_map<int, std::shared_ptr<socket_vc_connection>,
					phmap::priv::hash_default_hash<int>, phmap::priv::hash_default_eq<int>,
					phmap::priv::Allocator<std::pair<const int, std::shared_ptr<socket_vc_connection>>>, 4,
					std::mutex> users;

	db_connection_pool& pool;
	socket_main_server& sserv;

	std::string rtc_addr;
	int rtc_port;
	std::string rtc_cert, rtc_key;

	thread_pool thr_pool;
};

#endif
