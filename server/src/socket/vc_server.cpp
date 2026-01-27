#include "socket/vc_server.h"
#include "resource/channel.h"
#include "resource/utils.h"

#include <iostream>

/*** socket_vc_connection ***/

std::string socket_vc_connection::get_new_mid()
{
	return std::to_string(last_mid++);
}
void socket_vc_connection::change_track_desc(std::shared_ptr<rtc::Track> track, rtc::SSRC ssrc, int user_id)
{
	rtc::Description::Media desc = track->description();
	std::string mid = desc.mid();
	desc.clearSSRCs();
	desc.addSSRC(ssrc, mid);
	if(user_id != -1)
		desc.addAttribute("user:" + std::to_string(user_id));
	track->setDescription(desc);
}


nlohmann::json socket_vc_connection::get_vc_state()
{	
	return {
		{"id", user_id},
		{"mute", mute},
		{"deaf", deaf},
		{"video", get_recv_video_track() ? video_state::SCREEN : video_state::DISABLED}
	};
}

void socket_vc_connection::init_rtc(std::string rtc_addr, int rtc_port, std::string rtc_cert, std::string rtc_key)
{
	auto conf = rtc::Configuration();
	conf.portRangeBegin = conf.portRangeEnd = rtc_port;
	conf.enableIceUdpMux = true;
	conf.bindAddress = "0.0.0.0";
	conf.certificatePemFile = rtc_cert;
	conf.keyPemFile = rtc_key;
	rtc_conn = std::make_shared<rtc::PeerConnection>(conf);

	rtc_conn->onSignalingStateChange([this, rtc_addr](rtc::PeerConnection::SignalingState state){
		if(state == rtc::PeerConnection::SignalingState::HaveLocalOffer)
			// New local offer
			send_offer(rtc_addr);
	});
	rtc_conn->onGatheringStateChange([this, rtc_addr](rtc::PeerConnection::GatheringState state){
		if(state == rtc::PeerConnection::GatheringState::Complete)
			// Connection established
			send_offer(rtc_addr);
	});

	add_recv_audio_track();
}
void socket_vc_connection::send_offer(std::string rtc_addr)
{
	auto desc = rtc_conn->localDescription();
	// change candidates IPs to the specified public IP
	std::vector<rtc::Candidate> candidates = desc.value().extractCandidates();
	if(candidates.size() < 1)
		return; // candidates are not gathered yet (localDescription is set for the 1st time)
	candidates.push_back(candidates[0]);
	candidates[0].changeAddress(rtc_addr);
	candidates[1].changeAddress("127.0.0.1");
	desc.value().addCandidates(candidates);
	desc.value().endCandidates();

	socket_event ev;
	ev.name = "offer";
	ev.data = {
		{"type", desc->typeString()},
		{"sdp", std::string(desc.value())}
	};
	send(ev.dump());
}

void socket_vc_connection::add_audio_track(rtc::SSRC ssrc, int user_id)
{
	std::shared_ptr<rtc::Track> track;
	std::lock_guard lock(mut);

	if(removed_audio_tracks.size()){
		track = removed_audio_tracks.front();
		removed_audio_tracks.pop();
		change_track_desc(track, ssrc, user_id);
	} else {
		auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
		auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
		auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
		session_recv->addToChain(session_send);
	
		std::string mid = get_new_mid();
		rtc::Description::Audio desc(mid, rtc::Description::Direction::SendOnly);
		desc.addOpusCodec(RTC_PAYLOAD_TYPE_VOICE);
		desc.addSSRC(ssrc, mid);
		desc.addAttribute("user:" + std::to_string(user_id));

		track = rtc_conn->addTrack(desc);
		track->setMediaHandler(session_recv);
	}

	audio_tracks[user_id] = track;
}
void socket_vc_connection::remove_audio_track(int user_id)
{
	std::lock_guard lock(mut);
	if(audio_tracks.find(user_id) == audio_tracks.end())
		return;
	std::shared_ptr<rtc::Track> track = audio_tracks[user_id];

	rtc::Description::Media desc = track->description();
	desc.removeAttribute("user:" + std::to_string(user_id));
	track->setDescription(desc);

	removed_audio_tracks.push(track);
	audio_tracks.erase(user_id);
}
std::shared_ptr<rtc::Track> socket_vc_connection::get_audio_track(int user_id)
{
	std::lock_guard lock(mut);
	return audio_tracks.find(user_id) == audio_tracks.end() ? nullptr : audio_tracks[user_id];
		return nullptr;
}

void socket_vc_connection::add_video_track(rtc::SSRC ssrc, int user_id, codec cd, int bitrate)
{
	std::shared_ptr<rtc::Track> track;
	std::lock_guard lock(mut);

	if(removed_video_tracks.size()){
		track = removed_video_tracks.front();
		removed_video_tracks.pop();
		change_track_desc(track, ssrc, user_id);
	} else {
		auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "video", RTC_PAYLOAD_TYPE_VIDEO(cd), rtc::OpusRtpPacketizer::DefaultClockRate);
		auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
		auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
		session_recv->addToChain(session_send);
	
		std::string mid = get_new_mid();
		rtc::Description::Video desc(mid, rtc::Description::Direction::SendOnly);
		add_video_codec(desc, cd);
		desc.setBitrate(bitrate);
		desc.addSSRC(ssrc, mid);
		desc.addAttribute("user:" + std::to_string(user_id));

		track = rtc_conn->addTrack(desc);
		track->setMediaHandler(session_recv);
	}

	video_tracks[user_id] = track;
}
void socket_vc_connection::remove_video_track(int user_id)
{
	std::lock_guard lock(mut);
	if(video_tracks.find(user_id) == video_tracks.end())
		return;
	std::shared_ptr<rtc::Track> track = video_tracks[user_id];

	rtc::Description::Media desc = track->description();
	desc.removeAttribute("user:" + std::to_string(user_id));
	track->setDescription(desc);

	removed_video_tracks.push(track);
	video_tracks.erase(user_id);
}
std::shared_ptr<rtc::Track> socket_vc_connection::get_video_track(int user_id)
{
	std::lock_guard lock(mut);
	return video_tracks.find(user_id) == video_tracks.end() ? nullptr : video_tracks[user_id];
		return nullptr;
}

void socket_vc_connection::add_recv_audio_track()
{
	std::uniform_int_distribution<rtc::SSRC> dist(1, (rtc::SSRC)-1);
	const rtc::SSRC ssrc = dist(socket_vc_server::rng);
	auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
	auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
	auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
	session_recv->addToChain(session_send);

	rtc::Description::Audio desc("my_voice", rtc::Description::Direction::RecvOnly);
	desc.addOpusCodec(RTC_PAYLOAD_TYPE_VOICE);
	desc.addSSRC(ssrc, "my_voice");

	std::shared_ptr<rtc::Track> track = rtc_conn->addTrack(desc);
	std::lock_guard lock(mut);
	audio_tracks[-1] = track;
}

void socket_vc_connection::add_recv_video_track(codec cd, int bitrate)
{
	std::uniform_int_distribution<rtc::SSRC> dist(1, (rtc::SSRC)-1);
	const rtc::SSRC ssrc = dist(socket_vc_server::rng);
	std::shared_ptr<rtc::Track> track;
	std::lock_guard lock(mut);

	if(video_tracks.find(-1) != video_tracks.end()){
		track = video_tracks[-1];
		change_track_desc(track, ssrc);
		if(track->isOpen())
			track->requestKeyframe();
	} else {
		auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "video", RTC_PAYLOAD_TYPE_VIDEO(cd), rtc::OpusRtpPacketizer::DefaultClockRate);
		auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
		auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
		session_recv->addToChain(session_send);
	
		rtc::Description::Video desc("my_video", rtc::Description::Direction::RecvOnly);
		add_video_codec(desc, cd);
		desc.setBitrate(bitrate);
		desc.addSSRC(ssrc, "my_video");

		track = rtc_conn->addTrack(desc);
		track->setMediaHandler(session_recv);
		video_tracks[-1] = track;

		track->onOpen([track](){
			track->requestKeyframe();
		});
	}
	video_recv_is_open = true;
}
void socket_vc_connection::remove_recv_video_track()
{
	video_recv_is_open = false;
}
std::shared_ptr<rtc::Track> socket_vc_connection::get_recv_video_track()
{
	std::lock_guard lock(mut);
	return video_recv_is_open ? video_tracks[-1] : nullptr;
}

void socket_vc_connection::set_recv_video_bitrate(int bitrate)
{
	std::lock_guard lock(mut);
	get_recv_video_track()->requestBitrate(bitrate);
}

void socket_vc_connection::request_keyframe(int user_id)
{
	std::lock_guard lock(mut);
	requested_keyframes.insert(user_id);
}
bool socket_vc_connection::is_keyframe_requested(int user_id)
{
	std::lock_guard lock(mut);
	return requested_keyframes.erase(user_id);
}

rtc::SSRC socket_vc_connection::get_ssrc(std::shared_ptr<rtc::Track> track)
{
	return track->description().getSSRCs()[0];
}
codec socket_vc_connection::get_video_codec(std::shared_ptr<rtc::Track> tr)
{
	if(!tr)
		return codec::INVALID;
	return static_cast<codec>(tr->description().payloadTypes()[0] - RTC_PAYLOAD_TYPE_VIDEO(0));
}
void socket_vc_connection::add_video_codec(rtc::Description::Video& desc, codec cd)
{
	switch(cd){
		case codec::H264:
			desc.addH264Codec(RTC_PAYLOAD_TYPE_VIDEO(cd));
			break;
		case codec::VP8:
			desc.addVP8Codec(RTC_PAYLOAD_TYPE_VIDEO(cd));
			break;
	}
}

std::mutex& socket_vc_connection::get_mutex()
{
	return mut;
}

/*** socket_vc_channel ***/

void socket_vc_channel::add_user(std::shared_ptr<socket_vc_connection> conn, thread_pool& thr_pool)
{
	for_each([conn](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn){
		// Create send tracks for this connection
		conn->add_audio_track(other_conn->get_ssrc(other_conn->get_audio_track(-1)), other_user_id);
		std::shared_ptr<rtc::Track> other_recv_video_track = other_conn->get_recv_video_track();
		if(other_recv_video_track){
			conn->add_video_track(other_conn->get_ssrc(other_recv_video_track), other_user_id,
						other_conn->get_video_codec(other_recv_video_track), other_recv_video_track->description().bitrate());
			other_conn->request_keyframe(conn->user_id);
		}

		// Create send tracks for other connection
		other_conn->add_audio_track(conn->get_ssrc(conn->get_audio_track(-1)), conn->user_id);
		// video tracks are added after connection is established, so they cant be present here
		other_conn->rtc_conn->setLocalDescription();
	});
	conn->rtc_conn->setLocalDescription();

	{
		std::shared_ptr<rtc::Track> recv_audio = conn->get_audio_track(-1);
		recv_audio->onMessage([this, conn, recv_audio, &thr_pool](rtc::binary message) mutable {
			thr_pool.execute([this, conn, recv_audio, &thr_pool, message]() mutable {
				for_each([conn, &message, recv_audio](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn){
					if(conn == other_conn)
						return;
					std::shared_ptr<rtc::Track> send_track = other_conn->get_audio_track(conn->user_id);
					if(send_track){
						auto rtp = reinterpret_cast<rtc::RtpHeader*>(message.data());
						rtp->setSsrc(conn->get_ssrc(recv_audio));
						if(send_track->isOpen())
							send_track->send(message.data(), message.size());
					}
				});
			});
		}, nullptr);
	}

	std::lock_guard lock(mut);
	users[conn->user_id] = conn;
}
void socket_vc_channel::remove_user(std::shared_ptr<socket_vc_connection> conn)
{
	std::cerr << "REMOVING " << conn.get() << std::endl;
	for_each([conn](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn){
		if(other_conn == conn)
			return;
		other_conn->remove_audio_track(conn->user_id);
		other_conn->remove_video_track(conn->user_id);
		other_conn->rtc_conn->setLocalDescription();
	});

	std::lock_guard lock(mut);
	users.erase(conn->user_id);
	std::cerr << "REMOVED " << conn.get() << std::endl;
}
std::shared_ptr<socket_vc_connection> socket_vc_channel::get_user(int user_id)
{
	std::lock_guard lock(mut);
	return users.find(user_id) == users.end() ? nullptr : users[user_id];
}

void socket_vc_channel::enable_user_video(std::shared_ptr<socket_vc_connection> conn, thread_pool& thr_pool)
{
	for_each([conn](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn){
		if(conn == other_conn)
			return;

		std::shared_ptr<rtc::Track> recv_video_track = conn->get_recv_video_track();
		other_conn->add_video_track(conn->get_ssrc(recv_video_track), conn->user_id,
						conn->get_video_codec(recv_video_track), recv_video_track->description().bitrate());
		other_conn->rtc_conn->setLocalDescription();
	});
	conn->rtc_conn->setLocalDescription();

	{
		std::shared_ptr<rtc::Track> recv_video = conn->get_recv_video_track();
		recv_video->onMessage([this, conn, recv_video, &thr_pool](rtc::binary message){
			thr_pool.execute([this, conn, recv_video, &thr_pool, message]() mutable {
				for_each([conn, recv_video, &message](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn) mutable {
					if(conn == other_conn)
						return;
					std::shared_ptr<rtc::Track> send_track = other_conn->get_video_track(conn->user_id);
					if(send_track){
						auto rtp = reinterpret_cast<rtc::RtpHeader*>(message.data());
						rtp->setSsrc(conn->get_ssrc(recv_video));
						if(send_track->isOpen()){
							send_track->send(message.data(), message.size());
							if(conn->is_keyframe_requested(other_conn->user_id))
								recv_video->requestKeyframe();
						}
					}
				});
			});
		}, nullptr);
	}
}
void socket_vc_channel::disable_user_video(std::shared_ptr<socket_vc_connection> conn)
{
	for_each([conn](int other_user_id, std::shared_ptr<socket_vc_connection> other_conn){
		other_conn->remove_video_track(conn->user_id);
		other_conn->rtc_conn->setLocalDescription();
	});
}

void socket_vc_channel::for_each(std::function<void (int, std::shared_ptr<socket_vc_connection>)> cb)
{
	std::lock_guard lock(mut);
	for(auto it = users.begin(); it != users.end(); ++it)
		cb(it->first, it->second);
}


/*** socket_vc_server ***/

thread_local std::mt19937 socket_vc_server::rng{std::random_device{}()};

socket_vc_server::socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				std::string _rtc_addr, int _rtc_port): socket_server(https_key, https_cert, port, pool), pool{pool}, sserv{sserv}, rtc_addr{_rtc_addr}, rtc_port{_rtc_port}, rtc_cert{https_cert}, rtc_key{https_key}
{
	rtc::InitLogger(rtc::LogLevel::Debug);

	srv.setConnectionStateFactory([](){
		return std::make_shared<socket_vc_connection>();
	});

	srv.setOnConnectionCallback([this](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		std::shared_ptr<socket_vc_connection> conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);
		conn->sock = sock;

		sock.lock()->setOnMessageCallback([this, sock, _conn](const ix::WebSocketMessagePtr& msg){
			std::shared_ptr<socket_vc_connection> conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);

			if(msg->type == ix::WebSocketMessageType::Open){
				std::cerr << "got connection" << std::endl;
				auto query = parse_query(msg->openInfo.uri);

				// Validate connection
				{
					db_connection db_conn = this->pool.hold();
					pqxx::work tx{*db_conn};
					pqxx::result r;

					try {
						r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(parse_token(msg)));
					} catch(pqxx::data_exception& e){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
						return;
					}
					if(!r.size()){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
						return;
					}
					conn->user_id = r[0]["user_id"].as<int>();
	
					try{
						conn->channel_id = std::stol(query["channel"]);
					} catch(std::invalid_argument& e){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Couldn't parse channel ID, got '" + query["channel"] + "'");
						return;
					}

					r = tx.exec("SELECT type, server_id, channel_id FROM channels WHERE channel_id = $1 AND type = $2", pqxx::params(conn->channel_id, static_cast<int>(CHANNEL_VOICE)));
					if(!r.size()){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it does not exist");
						return;
					}
					conn->server_id = r[0]["server_id"].as<int>();
					if(!resource_utils::check_server_member(conn->user_id, conn->server_id, tx)){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it does not exist");
						return;
					}
				}
				std::cerr << "validated, user_id=" << conn->user_id << " channel_id=" << conn->channel_id << " server_id=" << conn->server_id << std::endl;

				// Check for already existing connections
				bool already_connected = false;
				channels.if_contains(conn->channel_id, [conn, &already_connected](std::pair<int, std::shared_ptr<socket_vc_channel>> p){
					if(p.second->get_user(conn->user_id))
						already_connected = true;
				});
				if(already_connected){
					conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User is already connected to this channel");
					return;
				}

				std::shared_ptr<socket_vc_connection> old_conn;
				users.if_contains(conn->user_id, [&old_conn](std::pair<int, std::shared_ptr<socket_vc_connection>> p){
					old_conn = p.second;
				});
				if(old_conn)
					old_conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User is connecting to a different channel");
				users[conn->user_id] = conn;
				std::cerr << "checked for existing connections" << std::endl;

				// Parse state
				if(!parse_audio_state(query, "mute", conn->mute)){
					conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid mute state '" + query["mute"] + "'");
					return;
				}
				if(!parse_audio_state(query, "deaf", conn->deaf)){
					conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid deaf state '" + query["deaf"] + "'");
					return;
				}
				std::cerr << "parsed state" << std::endl;

				conn->init_rtc(rtc_addr, rtc_port, rtc_cert, rtc_key);
				std::cerr << "initialized rtc" << std::endl;

				channels.emplace(conn->channel_id, std::make_shared<socket_vc_channel>());
				channels[conn->channel_id]->add_user(conn, thr_pool);
				std::cerr << "added user to channel" << std::endl;

				conn->rtc_conn->onStateChange([this, conn](rtc::PeerConnection::State state){
					if(state == rtc::PeerConnection::State::Connected){
						db_connection db_conn = this->pool.hold();
						pqxx::work tx{*db_conn};
						socket_event ev;
						ev.data = conn->get_vc_state();
						resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
						ev.name = "user_joined_vc";
						this->sserv.send_to_channel(conn->channel_id, tx, ev);
					} else if(state == rtc::PeerConnection::State::Failed){
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "ICE failed");
						conn->rtc_conn->close();
					} else if(state == rtc::PeerConnection::State::Closed)
						conn->close(ix::WebSocketCloseConstants::kNormalClosureCode, "RTC closed");
				});
			} else if(msg->type == ix::WebSocketMessageType::Close){
				// TODO change to code?
				if(msg->closeInfo.reason == "User is already connected to this channel")
					return;
				
				std::cerr << "removing user " << conn->user_id << std::endl;
				channels[conn->channel_id]->remove_user(conn);
				std::cerr << "removed user " << conn->user_id << std::endl;

				db_connection db_conn = this->pool.hold();
				pqxx::work tx{*db_conn};
				socket_event ev;
				resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
				ev.data["id"] = conn->user_id;
				ev.name = "user_left_vc";
				this->sserv.send_to_channel(conn->channel_id, tx, ev);

				// TODO change to code?
				if(msg->closeInfo.reason != "User is connecting to a different channel")
					users.erase(conn->user_id);
			} else if(msg->type == ix::WebSocketMessageType::Message){
				socket_event ev;
				try{
					ev = socket_event(msg->str);
				} catch(nlohmann::json::parse_error& e) {
					ev.name = "error";
					ev.data = "invalid JSON data";
					conn->send(ev.dump());
					return;
				}

				if(ev.name == "offer"){
					if(conn->rtc_conn->signalingState() != rtc::PeerConnection::SignalingState::Stable){
						rtc::Description answer(ev.data["sdp"].get<std::string>(), ev.data["type"].get<std::string>());
						conn->rtc_conn->setRemoteDescription(answer);
					}
				} else if(ev.name == "change_state"){
					bool changed = false;
					if(ev.data.contains("mute")){
						if(!ev.data["mute"].is_number_unsigned() || !check_audio_state(ev.data["mute"])){
							socket_event ev;
							ev.name = "error";
							ev.data = "invalid mute state";
							conn->send(ev.dump());
							return;
						}
						if(ev.data["mute"] != conn->mute){
							changed = true;
							conn->mute = ev.data["mute"];
						}
					}
					if(ev.data.contains("deaf")){
						if(!ev.data["deaf"].is_number_unsigned() || !check_audio_state(ev.data["deaf"])){
							socket_event ev;
							ev.name = "error";
							ev.data = "invalid deaf state";
							conn->send(ev.dump());
							return;
						}
						if(ev.data["deaf"] != conn->deaf){
							changed = true;
							conn->deaf = ev.data["deaf"];
						}
					}

					if(changed){
						db_connection db_conn = this->pool.hold();
						pqxx::work tx{*db_conn};

						socket_event ev;
						ev.data = conn->get_vc_state();
						resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
						ev.name = "user_changed_vc_state";
						this->sserv.send_to_channel(conn->channel_id, tx, ev);
					}
				} else if(ev.name == "enable_video"){
					std::cerr << "ENABLING VIDEO" << std::endl;
					if(conn->get_recv_video_track())
						return;

					codec cd = codec::H264;
					if(ev.data.contains("codec")){
						if(!ev.data["codec"].is_string() || 
							(cd = check_codec(ev.data["codec"])) == codec::INVALID){
							socket_event ev;
							ev.name = "error";
							ev.data = "invalid codec";
							conn->send(ev.dump());
							return;
						}
					}

					int bitrate = max_video_bitrate;
					if(ev.data.contains("bitrate")){
						if(!ev.data["bitrate"].is_number_unsigned()){
							socket_event ev;
							ev.name = "error";
							ev.data = "invalid bitrate";
							conn->send(ev.dump());
							return;
						}
						bitrate = std::min(ev.data["bitrate"].get<int>(), max_video_bitrate);
					}

					conn->add_recv_video_track(cd, bitrate);
					channels[conn->channel_id]->enable_user_video(conn, thr_pool);

					db_connection db_conn = this->pool.hold();
					pqxx::work tx{*db_conn};
					socket_event ev;
					ev.data = conn->get_vc_state();
					resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
					ev.name = "user_changed_vc_state";
					this->sserv.send_to_channel(conn->channel_id, tx, ev);
				} else if(ev.name == "disable_video") {
					if(!conn->get_recv_video_track())
						return;

					conn->remove_recv_video_track();
					channels[conn->channel_id]->disable_user_video(conn);

					db_connection db_conn = this->pool.hold();
					pqxx::work tx{*db_conn};
					socket_event ev;
					ev.data = conn->get_vc_state();
					resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
					ev.name = "user_changed_vc_state";
					this->sserv.send_to_channel(conn->channel_id, tx, ev);
				}
			}
		});
	});
}


void socket_vc_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	channels.if_contains(channel_id, [&dumped](std::pair<int, std::shared_ptr<socket_vc_channel>> p){
		p.second->for_each([&dumped](int, std::shared_ptr<socket_vc_connection> conn){
			conn->send(dumped);
		});
	});
}

nlohmann::json socket_vc_server::get_channel_users(int channel_id)
{
	std::cerr << "getting channel users for " << channel_id << std::endl;
	nlohmann::json out = nlohmann::json::array();
	channels.if_contains(channel_id, [&out](std::pair<int, std::shared_ptr<socket_vc_channel>> p){
		p.second->for_each([&out](int user_id, std::shared_ptr<socket_vc_connection> conn){
			out += conn->get_vc_state();
		});
	});
	std::cerr << "got users for " << channel_id << std::endl;
	return out;
}

codec socket_vc_server::check_codec(std::string str)
{
	static std::unordered_map<std::string, codec> _str_to_codec = {
		{"H264", codec::H264},
		{"VP8", codec::VP8}
	};
	if(_str_to_codec.find(str) == _str_to_codec.end())
		return codec::INVALID;
	return _str_to_codec[str];
}

bool socket_vc_server::parse_audio_state(std::unordered_map<std::string, std::string>& query, std::string arg_name, audio_state& out)
{
	if(query.find(arg_name) == query.end())
		return true;
	try{
		out = static_cast<audio_state>(std::stol(query[arg_name]));
	} catch(std::invalid_argument& e){
		return false;
	}
	return check_audio_state(out);
}
bool socket_vc_server::check_audio_state(unsigned state)
{
	return state >= audio_state::NO && state < audio_state::BY_ADMIN;
}
