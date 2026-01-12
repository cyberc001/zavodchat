#include "socket/vc_server.h"
#include "resource/server_channels.h"
#include "resource/utils.h"

#include <iostream>

#define SWAP32(x) ((((x)&0xFF000000) >> 24) | (((x)&0xFF) << 24) | (((x)&0xFF00) << 8) | (((x)&0xFF0000) >> 8))

size_t socket_vc_connection::add_audio_track(rtc::SSRC ssrc, int user_id)
{
	if(!unused_audio_tracks.empty()){
		size_t i = unused_audio_tracks.top();
		unused_audio_tracks.pop();
		user_to_audio_track[user_id] = i;
		rtc::Description::Media desc = tracks[i]->description();
		desc.clearSSRCs();
		desc.addSSRC(ssrc, std::to_string(i));
		desc.addAttribute("user:" + std::to_string(user_id));
		tracks[i]->setDescription(desc);
		return i;
	}
	auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
	auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
	auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
	session_recv->addToChain(session_send);

	size_t i = tracks.size();
	rtc::Description::Audio desc(std::to_string(i), rtc::Description::Direction::SendOnly);
	desc.addOpusCodec(RTC_PAYLOAD_TYPE_VOICE);
	desc.addSSRC(ssrc, std::to_string(i));
	desc.addAttribute("user:" + std::to_string(user_id));

	tracks.push_back(rtc_conn->addTrack(desc));
	tracks[i]->setMediaHandler(session_recv);
	user_to_audio_track[user_id] = i;
	return i;
}
void socket_vc_connection::remove_audio_track(int user_id)
{
	size_t i = user_to_audio_track[user_id];
	// remove user_id
	rtc::Description::Media desc = tracks[i]->description();
	desc.removeAttribute("user:" + std::to_string(user_id));
	tracks[i]->setDescription(desc);
	unused_audio_tracks.push(i);
	user_to_audio_track.erase(user_id);
}

size_t socket_vc_connection::add_recv_video_track(rtc::SSRC ssrc)
{
	recv_video_track_closed = false;
	if(user_to_video_track.find(-1) != user_to_video_track.end()){
		size_t i = user_to_video_track[-1];
		rtc::Description::Media desc = tracks[i]->description();
		desc.clearSSRCs();
		desc.addSSRC(ssrc, std::to_string(i));
		tracks[i]->setDescription(desc);

		users_needing_keyframe.insert(-1);
		return i;
	}
	size_t i = tracks.size();
	rtc::Description::Video desc("my_video", rtc::Description::Direction::RecvOnly);
	desc.addH264Codec(RTC_PAYLOAD_TYPE_VIDEO);
	desc.setBitrate(8000*1024);
	desc.addSSRC(ssrc, std::to_string(i));
	tracks.push_back(rtc_conn->addTrack(desc));
	user_to_video_track[-1] = i;
	return i;
}
bool socket_vc_connection::has_active_recv_video_track()
{
	return user_to_video_track.find(-1) != user_to_video_track.end() && !recv_video_track_closed;
}
size_t socket_vc_connection::add_video_track(rtc::SSRC ssrc, int user_id)
{
	if(!unused_video_tracks.empty()){
		size_t i = unused_video_tracks.top();
		unused_video_tracks.pop();
		user_to_video_track[user_id] = i;
		rtc::Description::Media desc = tracks[i]->description();
		desc.clearSSRCs();
		desc.addSSRC(ssrc, std::to_string(i));
		desc.addAttribute("user:" + std::to_string(user_id));
		tracks[i]->setDescription(desc);
		return i;
	}
	auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
	auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
	auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
	session_recv->addToChain(session_send);

	size_t i = tracks.size();
	std::cerr << "USING NEW VIDEO TRACK " << i << " FOR USER " << user_id << std::endl;
	rtc::Description::Video desc(std::to_string(i), rtc::Description::Direction::SendOnly);
	desc.addH264Codec(RTC_PAYLOAD_TYPE_VIDEO);
	desc.setBitrate(8000*1024);
	desc.addSSRC(ssrc, std::to_string(i));
	desc.addAttribute("user:" + std::to_string(user_id));

	tracks.push_back(rtc_conn->addTrack(desc));
	user_to_video_track[user_id] = i;
	tracks[i]->setMediaHandler(session_recv);
	return i;
}
void socket_vc_connection::remove_video_track(int user_id)
{
	if(user_to_video_track.find(user_id) == user_to_video_track.end())
		return;
	size_t i = user_to_video_track[user_id];
	// remove user_id
	rtc::Description::Media desc = tracks[i]->description();
	desc.removeAttribute("user:" + std::to_string(user_id));
	tracks[i]->setDescription(desc);
	unused_video_tracks.push(i);
	user_to_video_track.erase(user_id);
}
void socket_vc_channel::add_user(int user_id, std::shared_ptr<socket_vc_connection> conn)
{
	std::unique_lock lock(connections_mutex);
	connections[user_id] = conn;
}
void socket_vc_channel::remove_user(int user_id)
{
	std::unique_lock lock(connections_mutex);

	auto conn = connections[user_id].lock();

	connections.erase(user_id);
}
bool socket_vc_channel::has_user(int user_id)
{
	std::unique_lock lock(connections_mutex);
	return connections.find(user_id) != connections.end();
}
std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator socket_vc_channel::connections_begin() const { return connections.cbegin(); }
std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator socket_vc_channel::connections_end() const { return connections.cend(); }

socket_vc_server::socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				std::string _rtc_addr, int _rtc_port): socket_server(https_key, https_cert, port, pool), pool{pool}, sserv{sserv}, rneng(rndev()), rndist(1, (uint32_t)-1), rtc_addr{_rtc_addr}, rtc_port{_rtc_port}, rtc_cert{https_cert}, rtc_key{https_key}
{
	// TODO maybe initialization should be moved, but only one socket_vc_server instance is needed anyway
	rtc::InitLogger(rtc::LogLevel::Debug);
	
	srv.setConnectionStateFactory([](){
		return std::make_shared<socket_vc_connection>();
	});

	srv.setOnConnectionCallback([&](std::weak_ptr<ix::WebSocket> sock, std::shared_ptr<ix::ConnectionState> _conn){
		socket_vc_connection& conn = dynamic_cast<socket_vc_connection&>(*_conn);
		conn.sock = sock;

		sock.lock()->setOnMessageCallback([&, sock, _conn](const ix::WebSocketMessagePtr& msg){
			socket_vc_connection& conn = dynamic_cast<socket_vc_connection&>(*_conn);

			if(msg->type == ix::WebSocketMessageType::Open){
				/**** check database validity ****/
				int ch_type;
				{
					auto query = parse_query(msg->openInfo.uri);
					db_connection db_conn = pool.hold();
					pqxx::work tx{*db_conn};
					pqxx::result r;

					// check auth token
					try{
						r = tx.exec("SELECT user_id FROM sessions WHERE token = $1 AND expiration_time > now()", pqxx::params(parse_token(msg)));
					} catch(pqxx::data_exception& e){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid token");
						return;
					}
					if(!r.size()){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Invalid or expired token");
						return;
					}
					// check voice channel
					try{
						conn.channel_id = std::stol(query["channel"]);
					} catch(std::invalid_argument& e){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Couldn't parse channel ID, got '" + query["channel"] + "'");
						return;
					}
					conn.user_id = r[0]["user_id"].as<int>();

					// check channel type
					r = tx.exec("SELECT type, server_id, channel_id FROM channels WHERE channel_id = $1", pqxx::params(conn.channel_id));
					if(!r.size()){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it doesn't exist");
						return;
					}
					conn.server_id = r[0]["server_id"].as<int>();
					ch_type = r[0]["type"].as<int>();
					if(!resource_utils::check_server_member(conn.user_id, conn.server_id, tx)){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it doesn't exist");
						return;
					}
				}

				if(ch_type != CHANNEL_VOICE){
					conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "Isn't a voice channel");
					return;
				}

				// check if user isn't already connected
				{
					std::unique_lock lock(channels_mutex);
					if(channels[conn.channel_id].has_user(conn.user_id)){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User is already connected");
						return;
					}
				}
				{
					std::unique_lock lock(users_mutex);
					if(users.find(conn.user_id) != users.end() && !users[conn.user_id].lock()->sock.expired())
						users[conn.user_id].lock()->sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User connected to a different channel");
				}
				users[conn.user_id] = std::dynamic_pointer_cast<socket_vc_connection>(_conn);

				/**** connect RTC ****/
				auto conf = rtc::Configuration();
				conf.portRangeBegin = conf.portRangeEnd = rtc_port;
				conf.enableIceUdpMux = true;
				conf.bindAddress = "0.0.0.0";
				conf.certificatePemFile = rtc_cert;
				conf.keyPemFile = rtc_key;
				conn.rtc_conn = std::make_shared<rtc::PeerConnection>(conf);

				conn.rtc_conn->onSignalingStateChange([&conn, rtc_addr = this->rtc_addr](rtc::PeerConnection::SignalingState state){
					if(state == rtc::PeerConnection::SignalingState::HaveLocalOffer){
						auto desc = conn.rtc_conn->localDescription();
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
						conn.sock.lock()->send(ev.dump());
					}
				});
				conn.rtc_conn->onGatheringStateChange([&conn, rtc_addr = this->rtc_addr](rtc::PeerConnection::GatheringState state){
					if(state == rtc::PeerConnection::GatheringState::Complete){
						auto desc = conn.rtc_conn->localDescription();
						// change candidates IPs to the specified public IP
						std::vector<rtc::Candidate> candidates = desc.value().extractCandidates();
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
						conn.sock.lock()->send(ev.dump());
					}
				});

				const rtc::SSRC ssrc = rndist(rneng);
				auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
				auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
				auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
				session_recv->addToChain(session_send);

				// create receiving track
				rtc::Description::Audio desc("my_voice", rtc::Description::Direction::RecvOnly);
				desc.addOpusCodec(RTC_PAYLOAD_TYPE_VOICE);
				desc.addSSRC(ssrc, "my_voice");
				conn.tracks.push_back(conn.rtc_conn->addTrack(desc));
				// create send tracks for all already established connections
				{
					std::unique_lock lock(channels_mutex);
					socket_vc_channel& chan = channels[conn.channel_id];
					for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
						auto other_conn = it->second.lock();

						conn.add_audio_track(other_conn->tracks[0]->description().getSSRCs()[0], it->first);

						std::cerr << "ADDED ALREADY ESTABLISHED AUDIO TRACK " << conn.user_id << " " << it->first << " " << conn.tracks[conn.user_to_audio_track[it->first]]->description().getSSRCs()[0] << std::endl;

						if(other_conn->user_to_video_track.find(-1) == other_conn->user_to_video_track.end() || other_conn->recv_video_track_closed)
							continue; // this user didnt enable video
						conn.add_video_track(other_conn->tracks[other_conn->user_to_video_track[-1]]->description().getSSRCs()[0], it->first);
						std::cerr << "ADDED ALREADY ESTABLISHED VIDEO TRACK " << conn.user_id << " " << it->first << " " << conn.tracks[conn.user_to_video_track[it->first]]->description().getSSRCs()[0] << std::endl;
						other_conn->users_needing_keyframe.insert(conn.user_id);
					}
				}

				// renegotiate a new send track for all known connections
				{
					std::unique_lock lock(channels_mutex);
					socket_vc_channel& chan = channels[conn.channel_id];
					for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
						auto other_conn = it->second.lock();
						other_conn->add_audio_track(ssrc, conn.user_id);
						std::cerr << "ADDED AUDIO TRACK " << other_conn->user_id << " " << conn.user_id << " " << other_conn->tracks[other_conn->user_to_audio_track[conn.user_id]]->description().getSSRCs()[0] << std::endl;
						other_conn->rtc_conn->setLocalDescription();
						std::cerr << "NEW LOCAL DESC " << std::endl;
					}
				}

				conn.tracks[0]->setMediaHandler(session_recv);
				conn.tracks[0]->onMessage([&, _conn, ssrc](rtc::binary message){
					std::unique_lock lock(channels_mutex);
					auto& chan = channels[conn.channel_id];
					auto recv_conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);
					std::unique_lock conn_lock(chan.connections_mutex);
					//std::cerr << "AUDIO FROM " << recv_conn->user_id << " " << ssrc << std::endl;
					for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
						std::shared_ptr<socket_vc_connection> conn = it->second.lock();
						if(conn->user_id == recv_conn->user_id)
							continue;
						if(conn->user_to_audio_track.find(recv_conn->user_id) != conn->user_to_audio_track.end()){
							auto track = conn->tracks[conn->user_to_audio_track[recv_conn->user_id]];
							*(uint32_t*)(message.data() + 8) = SWAP32(ssrc); // change SSRC to the one specified in local description
							//std::cerr << "SEND AUDIO FROM " << recv_conn->user_id << " TO " << conn->user_id << " mid " << track->mid() << " ssrc " << ssrc << std::endl;
							track->send(message.data(), message.size());
						}
					}
				}, nullptr);

				conn.rtc_conn->setLocalDescription();

				// add to connections map when connection is established
				conn.rtc_conn->onStateChange([&, _conn](rtc::PeerConnection::State state){
					auto conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);
					std::cerr << "STATE CHANGE FOR VC USER " << conn->user_id << " TO " << state << std::endl;
					if(state == rtc::PeerConnection::State::Connected){
						{
							std::unique_lock lock(channels_mutex);
							channels[conn->channel_id].add_user(conn->user_id, conn);
						}

						db_connection db_conn = pool.hold();
						pqxx::work tx{*db_conn};
						socket_event ev;
						resource_utils::json_set_ids(ev.data, conn->server_id, conn->channel_id);
						ev.data["id"] = conn->user_id;
						ev.name = "user_joined_vc";
						sserv.send_to_channel(conn->channel_id, tx, ev);
					} else if(state == rtc::PeerConnection::State::Failed){
						if(!conn->sock.expired())
							conn->sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "ICE failed");
						conn->rtc_conn->close();
					} else if(state == rtc::PeerConnection::State::Closed){
						if(!conn->sock.expired())
							conn->sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "by_user");
					}
				});
			} else if(msg->type == ix::WebSocketMessageType::Close){
				if(msg->closeInfo.reason != "User is already connected"){
					if(conn.channel_id > 0){
						// remove audio and video tracks from already established connections
						{
							std::unique_lock lock(channels_mutex);
							socket_vc_channel& chan = channels[conn.channel_id];
							chan.remove_user(conn.user_id);
							for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
								auto other_conn = it->second.lock();
								other_conn->remove_audio_track(conn.user_id);
								other_conn->remove_video_track(conn.user_id);
								other_conn->rtc_conn->setLocalDescription();
								std::cerr << "(ON REMOVE) NEW LOCAL DESC " << std::endl;
							}
						}

						db_connection db_conn = pool.hold();
						pqxx::work tx{*db_conn};

						socket_event ev;
						resource_utils::json_set_ids(ev.data, conn.server_id, conn.channel_id);
						ev.data["id"] = conn.user_id;
						ev.name = "user_left_vc";
						sserv.send_to_channel(conn.channel_id, tx, ev);
					}

					if(msg->closeInfo.reason != "User connected to a different channel"){
						std::unique_lock lock(users_mutex);
						users.erase(conn.user_id);
					}
				}
			} else if(msg->type == ix::WebSocketMessageType::Message){
				socket_event ev;
				try{
					ev = socket_event(msg->str);
				} catch(nlohmann::json::parse_error& e) {
					ev.name = "error";
					ev.data = "invalid JSON data";
					conn.sock.lock()->send(ev.dump());
					return;
				}

				if(ev.name == "offer"){
					// Sometimes duplicate offers appear. Probably due to state changing to HaveLocalOffer more than one time. (Protect against malicious clients aswell)
					if(conn.rtc_conn->signalingState() != rtc::PeerConnection::SignalingState::Stable){
						rtc::Description answer(ev.data["sdp"].get<std::string>(), ev.data["type"].get<std::string>());
						conn.rtc_conn->setRemoteDescription(answer);
						std::cerr << "NEW REMOTE DESC " << conn.user_id << std::endl;
					}
				} else if(ev.name == "enable_video"){
					if(conn.has_active_recv_video_track()){
						socket_event ev;
						ev.name = "error";
						ev.data = "video is already enabled for this user";
						conn.sock.lock()->send(ev.dump());
						return;
					}

					if(ev.data.contains("bitrate")){
						if(!ev.data["bitrate"].is_number_unsigned()){
							socket_event ev;
							ev.name = "error";
							ev.data = "invalid bitrate";
							conn.sock.lock()->send(ev.dump());
							return;
						}
						conn.recv_video_track_bitrate = std::min(max_video_bitrate, ev.data["bitrate"].get<unsigned>());
					}
					else
						conn.recv_video_track_bitrate = max_video_bitrate;

					const rtc::SSRC ssrc = rndist(rneng);
					auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
					auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
					auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
					session_recv->addToChain(session_send);

					// renegotiate a new send track for all known connections
					{
						std::unique_lock lock(channels_mutex);
						socket_vc_channel& chan = channels[conn.channel_id];
						for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
							auto other_conn = it->second.lock();

							if(other_conn->user_id == conn.user_id)
								continue;
							other_conn->add_video_track(ssrc, conn.user_id);
							std::cerr << "ADDED VIDEO TRACK " << other_conn->user_id << " " << conn.user_id << " " << other_conn->tracks[other_conn->user_to_video_track[conn.user_id]]->description().getSSRCs()[0] << std::endl;
							other_conn->rtc_conn->setLocalDescription();
							std::cerr << "NEW LOCAL DESC " << other_conn->user_id << std::endl;
						}
					}

					size_t recv_i = conn.add_recv_video_track(ssrc);
					conn.tracks[recv_i]->setMediaHandler(session_recv);
					conn.tracks[recv_i]->onMessage([&, recv_i, _conn, ssrc](rtc::binary message){
						// TODO maybe optimize mutex
						std::unique_lock lock(channels_mutex);
						auto& chan = channels[conn.channel_id];
						auto recv_conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);
						if(!recv_conn->recv_video_track_requested_bitrate){
							recv_conn->recv_video_track_requested_bitrate = true;
							recv_conn->tracks[recv_i]->requestBitrate(recv_conn->recv_video_track_bitrate);
						}
						std::unique_lock conn_lock(chan.connections_mutex);
						for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
							std::shared_ptr<socket_vc_connection> conn = it->second.lock();
							if(conn->user_id == recv_conn->user_id)
								continue;
							if(conn->user_to_video_track.find(recv_conn->user_id) != conn->user_to_video_track.end()){
								auto track = conn->tracks[conn->user_to_video_track[recv_conn->user_id]];
								if(recv_conn->users_needing_keyframe.find(conn->user_id) != recv_conn->users_needing_keyframe.end()){
									recv_conn->tracks[recv_i]->requestKeyframe();
									recv_conn->users_needing_keyframe.erase(conn->user_id);
								}
								if(recv_conn->users_needing_keyframe.find(-1) != recv_conn->users_needing_keyframe.end()){
									recv_conn->tracks[recv_i]->requestKeyframe();
									recv_conn->users_needing_keyframe.erase(-1);
								}
								//std::cerr << "SEND VIDEO FROM " << recv_conn->user_id << " TO " << conn->user_id << " mid " << track->mid() << " ssrc " << ssrc << " ptr " << track << std::endl;
								auto rtp = reinterpret_cast<rtc::RtpHeader*>(message.data());
								rtp->setSsrc(ssrc);
								track->send(message.data(), message.size());
							}
						}
					}, nullptr);

					conn.rtc_conn->setLocalDescription();
				} else if(ev.name == "disable_video"){
					// remove video track for this user from all established connections
					{
						std::unique_lock lock(channels_mutex);
						socket_vc_channel& chan = channels[conn.channel_id];
						for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
							auto other_conn = it->second.lock();
							other_conn->remove_video_track(conn.user_id);
							other_conn->rtc_conn->setLocalDescription();
						}
						conn.recv_video_track_closed = true;
					}
				}
			}
		});
	});
}

void socket_vc_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	std::shared_lock lock(channels_mutex);
	socket_vc_channel& chan = channels[channel_id];
	std::unique_lock conn_lock(chan.connections_mutex);
	for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
		std::shared_ptr<ix::WebSocket> sock = it->second.lock()->sock.lock();
		if(sock)
			sock->send(dumped);
	}
}

void socket_vc_server::get_channel_users(int channel_id, std::vector<int>& users)
{
	std::shared_lock lock(channels_mutex);
	socket_vc_channel& chan = channels[channel_id];
	std::unique_lock conn_lock(chan.connections_mutex);
	for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it)
		users.push_back(it->first);
}
