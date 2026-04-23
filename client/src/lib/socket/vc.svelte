<script>
	import {PUBLIC_BASE_SOCKET_VC} from '$env/static/public';
	import {VCTrack, VCAudioTrack, VCVideoTrack, AudioState, VideoState} from '$lib/socket/vc_utils.svelte.js';
	import {asset} from '$app/paths';

	import Sound from '$lib/sound.js';

	import User from '$lib/rest/user.svelte.js';
	import Channel from '$lib/rest/channel.js';
	import BlockedUsers from '$lib/rest/blocked_users.js';
	import Preferences from '$lib/rest/preferences.svelte.js';
	
	let self_user = User.get(-1);
	let channel = $state();
	let blocked_users = BlockedUsers.get();
	$effect(() => {
		if(blocked_users.loaded)
			for(const user_id of Object.keys(audio))
				audio[user_id].set_blocked(blocked_users.data.findIndex((x) => x.id === parseInt(user_id)) !== -1);
	});

	let ws, rtc;
	let rtc_init = false;
	
	// key: mid
	let tracks = {};
	// key: user_id
	let audio = $state({});
	let video = $state({});
	
	let state = $state({text: "Waiting for first offer", color: "var(--clr_text_warning)"});
	const set_state = (text, color) => {
		state.text = text;
		state.color = `var(${color})`;
	}

	const ws_ping_intv = setInterval(() => {
		if(ws)
			ws.send(JSON.stringify({name: "ping", data: ""}));
	}, 60000);


	export function call(channel_id,
				onclose, onerror){
		channel = Channel.get(channel_id);
	
		ws = new WebSocket(PUBLIC_BASE_SOCKET_VC + "?channel=" + channel_id);
		in_call = true;

		ws.onclose = (e) => {
			clearInterval(ws_ping_intv);
			console.log("closing ws with rtc\n", rtc, "\n", e);
			if(rtc)
				rtc.close();
			// close all streams to make socket GC properly and prevent audio duplication
			if(video_send_track)
				video_send_track.stop();
			for(const track of Object.values(tracks))
				track.destroy();

			if(onclose)
				onclose(e);

			set_state(e.reason ? e.reason : "Disconnected", "--clr_text_error");
			Sound.play(asset("sounds/vc_leave.ogg"));

			rtc = undefined;
			tracks = {};
			audio = {}; video = {};
			ws = undefined;
		};
		ws.onerror = (e) => {
			if(onerror)
				onerrror(e);
		};
		ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			switch(_data.name){
				case "offer":
					handle_offer(_data.data);
					break;
			}
		};
	}
	
	export function end_call(){
		ws?.close();
		in_call = false;
	}
	
	
	// callback for MainSocket's onmessage
	export function on_main_message(name, data){
		const allowed_events = {"user_joined_vc": true,
					"user_left_vc": true,
					"user_changed_vc_state": true};
		if(!allowed_events[name] || data.channel_id !== channel?.data.id)
			return;

		switch(name){
			case "user_joined_vc":
				Sound.play(asset("sounds/vc_join.ogg"));
				break;
			case "user_left_vc":
				Sound.play(asset("sounds/vc_leave.ogg"));
				break;
			case "user_changed_vc_state":
				if(data.video === VideoState.Screen)
					Sound.play(asset("sounds/vc_video_enable.ogg"));
				if(data.id === self_user.data.id){
					if(data.mute === AudioState.BySelf || data.deaf === AudioState.BySelf)
						Sound.play(asset("sounds/vc_audio_mute.ogg"));
					else if(data.mute === AudioState.None || data.deaf === AudioState.None)
						Sound.play(asset("sounds/vc_audio_unmute.ogg"));
				}
				break;
		}
	}
	
	export function has_user(user_id){
		return channel.loaded && typeof(channel.data.vc_users[user_id]) !== "undefined";
	}

	// Getters

	let in_call = $state(false);
	export function is_in_call(){
		return in_call;
	}

	export function is_connected(){
		return state.color === "var(--clr_text_success)";
	}

	export function get_channel(){
		return channel;
	}

	export function get_state(){
		return state;
	}

	export function get_audio(user_id){
		return audio[user_id];
	}
	export function get_video(user_id){
		return video[user_id];
	}


	// Audio management
		
	let mute = $state(AudioState.None);
	export function is_mute(){
		return mute !== AudioState.None;
	}
	export function toggle_mute(){
		mute = AudioState.toggled(mute);
		audio[self_user.data.id].ctx_dest.stream.getTracks()[0].enabled = mute == AudioState.None;
		ws.send(JSON.stringify({"name": "change_state", "data": {"mute": mute}}));
	}

	let deaf = $state(AudioState.None);
	export function is_deaf(){
		return deaf !== AudioState.None;
	}
	export function toggle_deaf(){
		deaf = AudioState.toggled(deaf);
		for(const track of Object.values(audio))
			if(track.media)
				track.set_muted(deaf !== AudioState.None);
		ws.send(JSON.stringify({"name": "change_state", "data": {"deaf": deaf}}));
	}	

		
	let video_state = $state(VideoState.Disabled);
	export function get_video_state(){
		return video_state;
	}

	let pending_video_state = $state(VideoState.None);
	let is_video_enabled = $derived(video_state > VideoState);
	
	let video_send_track;
	export async function set_video_state(s){
		if(s === video_state)
			return;
	
		switch(s){
			case VideoState.Disabled:
				ws.send(JSON.stringify({name: "disable_video"}));
				break;
			case VideoState.Screen:
				try {
					const media = await navigator.mediaDevices.getDisplayMedia();
					video_send_track = media.getTracks()[0];
					video_send_track.onended = () => {
						if(ws.readyState === WebSocket.OPEN)
							set_video_state(VideoState.Disabled);
					}
					ws.send(JSON.stringify({
						name: "enable_video",
						data: {
							codec: (get_video_codecs()["video/VP8"] ? "VP8" : "H264")
						}
					}));
				} catch {
					// Dont change video state if screensharing was denied
					return;
				}
				break;
		}

		pending_video_state = s;
	}
	
	let watched_video = $state();
	export function get_watched_video(){
		return watched_video;
	}
	export function watch_video(user_id){
		watched_video = video[user_id].track;
	}
	export function unwatch_video(){
		watched_video = undefined;
	}
	
	// Helper methods

	const get_video_codecs = () => {
		const codecs = {};
		for(const c of RTCRtpReceiver.getCapabilities("video").codecs)
			codecs[c.mimeType] = true;
		return codecs;
	}
	
	const handle_offer = async (offer) => {
		if(!rtc){
			rtc = new RTCPeerConnection({bundlePolicy: "max-bundle"});
	
			// Report state changes
			rtc.onconnectionstatechange = () => {
				switch(rtc.connectionState){
					case "connecting":
						set_state("ICE is connecting", "--clr_text_warning");
						break;
					case "disconnected":
						set_state("ICE is disconnected", "--clr_text_error");
						break;
					case "failed":
						set_state("ICE has failed", "--clr_text_error");
						break;
					case "connected":
						set_state("Connected", "--clr_text_success");
						break;
				}
			};
			rtc.onsignalingstatechange = () => {
				switch(rtc.signalingState){
					case "have-local-offer":
						set_state("RTC has a local offer", "--clr_text_warning");
						break;
					case "have-remote-offer":
						set_state("RTC has a remote offer", "--clr_text_warning");
						break;
					case "stable":
						set_state("Connected", "--clr_text_success");
						break;
				}
			};

			// Handle new tracks added to connection
			rtc.ontrack = async (e) => {
				const id = e.transceiver.mid;

				//e.track.enabled = deaf === AudioState.None;

				let track;
				switch(e.track.kind){
					case "audio":
						track = tracks[id] = new VCAudioTrack();
						break;
					case "video":
						track = tracks[id] = new VCVideoTrack();
						break;
					default:
						console.warn(`Unsupported track kind '${e.track.kind}'`);
						break;
				
				}
				await track.init(e.track);
				console.log("added track", track);
			};

			// Handle local RTC being ready to send counteroffer
			rtc.onsignalingstatechange = (state) => {
				if(rtc.signalingState === "stable"){
					ws.send(JSON.stringify({"name": "offer", "data": rtc.localDescription}));
					console.log("sent counteroffer");
				}
			};

			console.log("TRY TO ADD MICROPHONE TRACK", Preferences);
			// Add microphone track
			try {
				const media = await navigator.mediaDevices.getUserMedia({audio:
					Preferences.data.noise_supression === "browser" ?
					true
					: {
						echoCancellation: false,
						autoGainControl: false,
						noiseSupression: false
					}
				});
				set_state("Initializing microphone", "--clr_text_warning");
				const new_track = media.getTracks()[0];
				
				let track = audio[self_user.data.id] = tracks[-1] = new VCAudioTrack();
				track.user_id = self_user.data.id;
				await track.init(new_track, true);
				rtc.addTrack(track.ctx_dest.stream.getTracks()[0], track.ctx_dest.stream);
				console.log("ADDED MICROPHONE TRACK", track);
			} catch(e) {
				console.log(e);
			}

			rtc_init = true;
			state.text = "Initializing RTC";
		} else if(!rtc_init) // duplicate offer
			return;

		console.log("got offer", offer);
		await rtc.setRemoteDescription(offer);
		parse_sdp_user_ids(offer.sdp);

		switch(pending_video_state){
			case VideoState.Disabled:
				for(const tr of rtc.getTransceivers())
					if(tr.mid === "my_video"){
						tr.sender.track.stop();
						await tr.sender.replaceTrack(null);

						video_send_track = undefined;
						video_state = VideoState.Disabled;
						pending_video_state = VideoState.None;
						break;
					}
				break;
			case VideoState.Screen:
				for(const tr of rtc.getTransceivers())
					if(tr.mid === "my_video" && !tr.sender.track){
						// Manually add video send track
						tr.sender.replaceTrack(video_send_track);
						// Manual corrections are needed because RTCPeerConnection thinks that
						// video tracks are bidirectional, even though they are sendonly
						tr.direction = "sendonly";

						video_state = VideoState.Screen;
						pending_video_state = VideoState.None;
						break;
					}
				break;
		}

		const answer = await rtc.createAnswer();
		await rtc.setLocalDescription(answer);
		console.log("set counteroffer", answer);
	}

	const parse_sdp_user_ids = (sdp) => {
		audio = {[self_user.data.id]: tracks[-1]};
		video = {};
		let remove_watch = true;

		// присутствуют атрибуты mid и user_id -> дорожка используется
		let lines = sdp.split('\n');
		let type, mid, user_id;
		for(let i = 0; i < lines.length; ++i){
			let l = lines[i];
			if(l[0] == 'm' || i == lines.length - 1){
				if(mid && user_id && type){
					mid = parseInt(mid);
					user_id = parseInt(user_id);

					tracks[mid].user_id = user_id;
					if(type.startsWith("video")){
						video[user_id] = tracks[mid];
						if(tracks[mid].track.id === watched_video?.id)
							remove_watch = false;
					}
					else if(type.startsWith("audio"))
						audio[user_id] = tracks[mid];

				}
				type = mid = user_id = undefined;
				if(l[0] == 'm')
					type = l.substring(2);
			} else if(l.startsWith('a=user:'))
				user_id = l.substring(7);
			else if(l.startsWith('a=mid:'))
				mid = l.substring(6);
		}

		if(remove_watch)
			unwatch_video();
	};
</script>
