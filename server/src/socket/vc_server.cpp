#include "socket/vc_server.h"
#include "resource/server_channels.h"
#include "resource/utils.h"
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include <iostream>

// TODO delete
void cb_message (GstBus* bus, GstMessage* msg, void* data)
{
	switch(GST_MESSAGE_TYPE(msg)){
		case GST_MESSAGE_ERROR:
			GError* err;
			gchar* debug_info;
			gst_message_parse_error(msg, &err, &debug_info);
			std::cerr << "Message " << GST_MESSAGE_TYPE(msg) << " from " << GST_MESSAGE_SRC_NAME(msg) << std::endl;
			std::cerr << "Error " << err->message << std::endl;
			std::cerr << "Debug information " << debug_info << std::endl;
			break;
	}
}
void gst_main_loop()
{
	GMainLoop* mloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mloop);
	g_main_loop_unref(mloop);
}

GstFlowReturn vc_appsink_new_sample_callback(GstElement* appsink, gpointer udata)
{
	socket_vc_channel* chan = reinterpret_cast<socket_vc_channel*>(udata); // TODO change to something else, maybe pointer to vc server

	GstSample* smpl = gst_app_sink_pull_sample(GST_APP_SINK(appsink));
	if(!smpl){
		std::cerr << "Couldn't pull sample on 'new-sample' event'" << std::endl;
		return GST_FLOW_ERROR;
	}
	GstBuffer* buf = gst_sample_get_buffer(smpl);
	GstMapInfo mapped_mem;
	if(gst_buffer_map(buf, &mapped_mem, GST_MAP_READ)){
		std::cerr << std::hex;
		for(size_t i = 8; i < 12; ++i) std::cerr << (unsigned)mapped_mem.data[i] << ' ';
		std::cerr << std::endl;

		//uint32_t ssrc = *(reinterpret_cast<uint32_t*>(mapped_mem.data + 4));
		//std::cerr << "ssrc " << ssrc << std::endl;

		for(auto it = chan->connections_begin(); it != chan->connections_end(); ++it){
			std::shared_ptr<socket_vc_connection> conn = it->second.lock();
			conn->track_voice->send(reinterpret_cast<const std::byte*>(mapped_mem.data), mapped_mem.size);
		}
	}

	gst_sample_unref(smpl);
	return GST_FLOW_OK;
}

void socket_vc_connection::close(GstElement* pipeline)
{
	if(!appsrc)
		return;

	gst_object_unref(muxer_sink);

	gst_element_set_state(appsrc, GST_STATE_NULL);
	if(pipeline)
		gst_bin_remove(GST_BIN(pipeline), appsrc);
	else
		gst_object_unref(appsrc);
}

void socket_vc_channel::add_user(int user_id, std::shared_ptr<socket_vc_connection> conn)
{
	connections[user_id] = conn;
	// TODO change to create pipeline if >1 users are present
	// TODO create better logging
	if(!pipeline){
		std::string pipeline_name = "vc_pipeline_" + std::to_string(conn->channel_id);
		pipeline = gst_pipeline_new(pipeline_name.c_str());
		if(!pipeline){
			std::cerr << "Cannot create pipeline for voice channel " << conn->channel_id << std::endl;
			return;
		}

		//TODO delete
		GstBus* bus = gst_element_get_bus(pipeline);
		gst_bus_add_signal_watch(bus);
		g_signal_connect(bus, "message", G_CALLBACK (cb_message), nullptr);

		muxer = gst_element_factory_make("rtpmux", "muxer");
		if(!muxer){
			std::cerr << "Cannot create muxer for voice channel " << conn->channel_id << std::endl;
			return;
		}
		gst_bin_add(GST_BIN(pipeline), muxer);

		appsink = gst_element_factory_make("appsink", "out");
		if(!appsink){
			std::cerr << "Cannot create appsink for voice channel " << conn->channel_id << std::endl;
			return;
		}
		gst_bin_add(GST_BIN(pipeline), appsink);
		if(!gst_element_link(muxer, appsink)){
			std::cerr << "Cannot link muxer to appsink for voice channel " << conn->channel_id << std::endl;
			return;
		}
		g_object_set(appsink, "emit-signals", 1, NULL);
		g_signal_connect(appsink, "new-sample", G_CALLBACK(vc_appsink_new_sample_callback), this);

		GstStateChangeReturn state_err = gst_element_set_state(pipeline, GST_STATE_PLAYING);
		if(state_err == GST_STATE_CHANGE_FAILURE){
			std::cerr << "Cannot set pipeline state to PLAYING for voice channel " << conn->channel_id << std::endl;
			return;
		}
	}

	if(!conn->appsrc){
		std::string appsrc_name = "appsrc_" + std::to_string(user_id);
		conn->appsrc = gst_element_factory_make("appsrc", appsrc_name.c_str());
		if(!conn->appsrc){
			std::cerr << "Cannot create appsrc for voice channel " << conn->channel_id << ", user " << user_id << std::endl;
			return;
		}
		GstCaps* appsrc_caps = gst_caps_new_empty_simple("application/x-rtp");
		gst_app_src_set_caps(GST_APP_SRC(conn->appsrc), appsrc_caps);
		gst_bin_add(GST_BIN(pipeline), conn->appsrc);

		conn->muxer_sink = gst_element_request_pad_simple(muxer, "sink_%u");
		if(!conn->muxer_sink){
			std::cerr << "Cannot request RTPMux sink for voice channel " << conn->channel_id << ", user " << user_id << std::endl;
			return;
		}

		GstPad* appsrc_src = gst_element_get_static_pad(conn->appsrc, "src");
		if(gst_pad_link(appsrc_src, conn->muxer_sink))
			std::cerr << "Cannot link appsrc src to RTPMux sink for voice channel " << conn->channel_id << ", user " << user_id << std::endl;

		gst_element_set_state(conn->appsrc, GST_STATE_PLAYING);
	}
}
void socket_vc_channel::remove_user(int user_id)
{
	if(has_user(user_id)){
		connections[user_id].lock()->close(pipeline);
		connections.erase(user_id);
	}
	if(!connections.size() && pipeline){
		gst_element_set_state(pipeline, GST_STATE_NULL);
		gst_object_unref(pipeline);
		pipeline = nullptr;
	}
}
bool socket_vc_channel::has_user(int user_id) const { return connections.find(user_id) != connections.end(); }
std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator socket_vc_channel::connections_begin() const { return connections.cbegin(); }
std::unordered_map<int, std::weak_ptr<socket_vc_connection>>::const_iterator socket_vc_channel::connections_end() const { return connections.cend(); }

socket_vc_server::socket_vc_server(std::string https_key, std::string https_cert, int port,
				db_connection_pool& pool, socket_main_server& sserv,
				int rtc_port): socket_server(https_key, https_cert, port, pool), pool{pool}, sserv{sserv}, rneng(rndev()), rndist(1, (uint32_t)-1), rtc_port{rtc_port}, rtc_cert{https_cert}, rtc_key{https_key}
{
	// TODO maybe initialization should be moved, but only one socket_vc_server instance is needed anyway
	rtc::InitLogger(rtc::LogLevel::Debug);
	gst_init(0, NULL);
	gst_thr = std::thread(gst_main_loop);
	
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
				{
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
					int ch_type = r[0]["type"].as<int>();
					if(resource_utils::check_server_member(conn.user_id, conn.server_id, tx)){
						conn.sock.lock()->close(ix::WebSocketCloseConstants::kNormalClosureCode, "User has no access to the channel or it doesn't exist");
						return;
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
				}

				/**** connect RTC ****/
				auto conf = rtc::Configuration();
				conf.portRangeBegin = conf.portRangeEnd = rtc_port;
				conf.enableIceUdpMux = true;
				conf.bindAddress = "0.0.0.0";
				conf.certificatePemFile = rtc_cert;
				conf.keyPemFile = rtc_key;
				conn.rtc_conn = std::make_shared<rtc::PeerConnection>(conf);

				conn.rtc_conn->onGatheringStateChange([&conn](rtc::PeerConnection::GatheringState state){
					if(state == rtc::PeerConnection::GatheringState::Complete){
						auto desc = conn.rtc_conn->localDescription();
						nlohmann::json offer = {
							{"type", desc->typeString()},
							{"sdp", std::string(desc.value())}
						};
						conn.sock.lock()->send(offer.dump());
					}
				});

				const rtc::SSRC ssrc = rndist(rneng);
				rtc::Description::Audio desc("audio", rtc::Description::Direction::SendRecv);
				desc.addOpusCodec(RTC_PAYLOAD_TYPE_VOICE);
				desc.addSSRC(ssrc, "audio");
				conn.track_voice = conn.rtc_conn->addTrack(desc);

				auto rtp_conf = std::make_shared<rtc::RtpPacketizationConfig>(ssrc, "audio", RTC_PAYLOAD_TYPE_VOICE, rtc::OpusRtpPacketizer::DefaultClockRate);
				auto session_recv = std::make_shared<rtc::RtcpReceivingSession>();
				auto session_send = std::make_shared<rtc::RtcpSrReporter>(rtp_conf);
				session_recv->addToChain(session_send);
				conn.track_voice->setMediaHandler(session_recv);

				conn.track_voice->onMessage([&, _conn](rtc::binary message) {
					auto& chan = channels[conn.channel_id];
					auto recv_conn = std::dynamic_pointer_cast<socket_vc_connection>(_conn);
					for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it){
						std::shared_ptr<socket_vc_connection> conn = it->second.lock();
						if(conn->user_id == recv_conn->user_id)
							continue;
						conn->track_voice->send(message.data(), message.size());
					}
					/*GstBuffer* buf = gst_buffer_new_memdup(message.data(), message.size());
					GstFlowReturn flow;
					g_signal_emit_by_name(conn.appsrc, "push-buffer", buf, &flow);*/
				}, nullptr);

				conn.rtc_conn->setLocalDescription();

				// add to connections map when connection is established
				conn.rtc_conn->onStateChange([&](rtc::PeerConnection::State state){
					if(state == rtc::PeerConnection::State::Connected){
						{
							std::unique_lock lock(channels_mutex);
							channels[conn.channel_id].add_user(conn.user_id, std::dynamic_pointer_cast<socket_vc_connection>(_conn));
						}

						db_connection db_conn = pool.hold();
						pqxx::work tx{*db_conn};
						socket_event ev;
						resource_utils::json_set_ids(ev.data, conn.server_id, conn.channel_id);
						ev.data["id"] = conn.user_id;
						ev.name = "user_joined_vc";
						sserv.send_to_channel(conn.channel_id, tx, ev);	
					}
				});
			} else if(msg->type == ix::WebSocketMessageType::Close){
				if(msg->closeInfo.reason != "User is already connected"){
					if(conn.channel_id > 0){
						{
							std::unique_lock lock(channels_mutex);
							channels[conn.channel_id].remove_user(conn.user_id);
						}

						db_connection db_conn = pool.hold();
						pqxx::work tx{*db_conn};

						socket_event ev;
						resource_utils::json_set_ids(ev.data, conn.server_id, conn.channel_id);
						ev.data["id"] = conn.user_id;
						ev.name = "user_left_vc";
						sserv.send_to_channel(conn.channel_id, tx, ev);
					}
				}
			} else if(msg->type == ix::WebSocketMessageType::Message){
				nlohmann::json j = nlohmann::json::parse(msg->str);
				rtc::Description answer(j["sdp"].get<std::string>(), j["type"].get<std::string>());
				conn.rtc_conn->setRemoteDescription(answer);
				std::cerr << "got answer from client:" << std::endl << j << std::endl;
			}
		});
	});
}

void socket_vc_server::send_to_channel(int channel_id, pqxx::work& tx, socket_event event)
{
	std::string dumped = event.dump();

	std::shared_lock lock(channels_mutex);
	socket_vc_channel& chan = channels[channel_id];
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
	for(auto it = chan.connections_begin(); it != chan.connections_end(); ++it)
		users.push_back(it->first);
}
