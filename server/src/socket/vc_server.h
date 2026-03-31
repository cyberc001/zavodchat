#ifndef SOCKET_VC_SERVER_H
#define SOCKET_VC_SERVER_H

#include "socket/main_server.h"
#include "rtc/rtc.hpp"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <memory>
#include <queue>
#include <random>

#define RTC_PAYLOAD_TYPE_VOICE 96
#define RTC_PAYLOAD_TYPE_VIDEO(cd) (97 + (cd))
enum codec
{
	INVALID,
	H264, VP8
};

enum audio_state
{
	NO, SELF, BY_ADMIN
};
enum video_state
{
	DISABLED, SCREEN
};

class socket_vc_connection : public socket_connection
{
public:
	std::chrono::system_clock::time_point join_ts;
	int server_id = -1, channel_id = -1;
	std::shared_ptr<rtc::PeerConnection> rtc_conn;


	nlohmann::json get_vc_state();

	void init_rtc(std::string rtc_addr, int rtc_port, std::string rtc_cert, std::string rtc_key);

	void add_audio_track(rtc::SSRC, int user_id);
	void remove_audio_track(int user_id);
	std::shared_ptr<rtc::Track> get_audio_track(int user_id);

	void add_video_track(rtc::SSRC, int user_id, codec cd, int bitrate);
	void remove_video_track(int user_id);
	std::shared_ptr<rtc::Track> get_video_track(int user_id);

	void add_recv_video_track(codec cd, int bitrate);
	void remove_recv_video_track();
	// Returns nullptr if recv track does not exist or is closed
	std::shared_ptr<rtc::Track> get_recv_video_track();
	void set_recv_video_bitrate(int bitrate);

	void request_keyframe(int send_user_id);
	void send_keyframes();
	bool needs_keyframe();
	bool needs_keyframe(int send_user_id); // also erases send_user_id from the se

	rtc::SSRC get_ssrc(std::shared_ptr<rtc::Track>);
	codec get_video_codec(rtc::shared_ptr<rtc::Track>);
	void add_video_codec(rtc::Description::Video&, codec);

	std::mutex& get_mutex();

	audio_state mute = audio_state::NO, deaf = audio_state::NO;

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

	static const unsigned keyframe_interval = 5000;
	std::chrono::time_point<std::chrono::high_resolution_clock> last_keyframe_tp;
	// key: user_id
	std::unordered_set<int> new_users_for_keyframes;
	
	// This track shouldnt be removed
	void add_recv_audio_track();

	int video_recv_bitrate = -1;
	bool video_recv_is_open = false;
};

class socket_vc_server;

class socket_vc_channel
{
public:
	socket_vc_channel(socket_vc_server& vcserv);

	void add_user(std::shared_ptr<socket_vc_connection>);
	void remove_user(std::shared_ptr<socket_vc_connection>);
	std::shared_ptr<socket_vc_connection> get_user(int user_id);

	void enable_user_video(std::shared_ptr<socket_vc_connection>);
	void disable_user_video(std::shared_ptr<socket_vc_connection>);

	std::vector<std::shared_ptr<socket_vc_connection>> get_users();
	size_t get_user_count();

	bool is_inactive();
private:
	socket_vc_server& vcserv;

	// Last time point when two or more users were present; used in private calls
	std::chrono::system_clock::time_point busy_ts = std::chrono::system_clock::now();

	std::mutex mut;
	std::unordered_map<int, std::shared_ptr<socket_vc_connection>> users;

	static const size_t inactive_channel_time = 10000;
};

class socket_vc_server : public socket_server
{
public:
	static thread_local std::mt19937 rng;

	socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				std::string rtc_addr, int rtc_port);

	void close_channel(int channel_id, std::string reason);

	void send_to_channel(int channel_id, pqxx::work& tx, socket_event event); // only sends event to users currently connected to voice channel
	
	nlohmann::json get_channel_users(int channel_id); // get users connected to voice channel


	int max_video_bitrate = 10240000;
private:
	codec check_codec(std::string);

	bool parse_audio_state(std::unordered_map<std::string, std::string>& query, std::string arg_name, audio_state& out);
	bool check_audio_state(unsigned state);

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

	std::thread inactive_channel_thr;
	static const size_t inactive_channel_check_period = 1000;
};

#endif
