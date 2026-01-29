import {PUBLIC_BASE_SOCKET_VC} from '$env/static/public';
import Channel from '$lib/rest/channel.js';
import Preferences from '$lib/rest/preferences.svelte.js';
import Sound from '$lib/sound.js';

class VCTrack {
	user_id;
	track; media;
	volume = $state(1);

	set_volume(vol){
		if(this.media){
			this.volume = vol;
			this.media.volume = vol;
		}
	}

	async init(track){
		this.track = track;
	}
	destroy(){
		if(this.track)
			this.track.stop();
		if(this.media)
			this.media.srcObject = null;
	}
}

class VCAudioTrack extends VCTrack {
	ctx; analyser;
	ctx_src; ctx_dest;
	amplitude = $state(0);

	async init(track, mic){
		super.init(track);
		if(!mic){
			this.media = new Audio();
			this.media.srcObject = new MediaStream([track]);
		}

		this.ctx = new AudioContext();
		this.ctx_src = this.ctx.createMediaStreamSource(mic ? new MediaStream([track]) : this.media.srcObject);

		this.analyser = this.ctx.createAnalyser();
		this.analyser.smoothingTimeConstant = 0;

		if(mic){
			this.ctx_dest = this.ctx.createMediaStreamDestination();
			if(Preferences.data.noise_supression === "rnnoise"){
				await this.ctx.audioWorklet.addModule("/src/lib/socket/vc_mic_processor.js");
				this.processor = new AudioWorkletNode(this.ctx, "vc_mic_processor");
				this.ctx_src.connect(this.processor).connect(this.ctx_dest);
				this.processor.connect(this.analyser);
			} else {
				this.ctx_src.connect(this.ctx_dest);
			}
		} else
			this.ctx_src.connect(this.analyser);

		VCAudioTrack._glbl_tracks[this.track.id] = this;

		if(!mic)
			this.media.play();
	}

	static _glbl_tracks = {};
	static _glbl_volume_intv = setInterval(() => {
		for(const id of Object.keys(VCAudioTrack._glbl_tracks)){
			let track = VCAudioTrack._glbl_tracks[id];

			if(track.analyser){
				if(track.processor && !track.ctx_dest.stream.getTracks()[0].enabled){
					// Muted microphone track
					track.amplitude = 0;
					continue;
				}

				const samples = new Uint8Array(track.analyser.frequencyBinCount);
				track.analyser.getByteFrequencyData(samples);
				let sum = 0;
				for(let k = 0; k < samples.length; ++k)
					sum += samples[k];
				sum /= samples.length;

				track.amplitude = sum > 1 ? sum : 0;
			}
		}
	}, 100);

	destroy(){
		delete VCAudioTrack._glbl_tracks[this.track.id];

		// Dereference all AudioNodes before closing AudioContext so they get released
		// TODO Chromium still leaks memory due to a bug
		this.analyser = this.ctx_src = this.ctx_dest = null;
		if(this.ctx)
			this.ctx.close();

		super.destroy();
	}
}
class VCVideoTrack extends VCTrack {
}

export default class VCSocket {
	user_id;
	channel;

	ws;
	rtc; rtc_init = false;

	// key: mid
	tracks = {};
	// key: user_id
	audio = $state({});
	video = $state({});

	state = $state({text: "Waiting for first offer", color: "var(--clr_text_warning)"});
	set_state(text, color){
		this.state.text = text;
		this.state.color = `var(${color})`;
	}

	constructor(user_id, channel_id,
			onclose, onerror){
		this.user_id = user_id;
		this.channel = Channel.get(channel_id);

		this.ws = new WebSocket(PUBLIC_BASE_SOCKET_VC + "?channel=" + channel_id);
		this.ws.onclose = (e) => {
			if(this.rtc)
				this.rtc.close();
			// close all streams to make socket GC properly and prevent audio duplication
			if(this.video_send_track)
				this.video_send_track.stop();
			for(const track of Object.values(this.tracks))
				track.destroy();
			onclose(e);
			Sound.play("/src/lib/assets/sounds/vc_leave.ogg");
		}
		this.ws.onerror = onerror;
		this.ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			console.log("message", _data);

			switch(_data.name){
				case "offer":
					this.handle_offer(_data.data);
					break;
			}
		};
	}

	end_call(){
		this.ws.close();
	}


	// callback for MainSocket's onmessage
	on_main_message(name, data){
		const allowed_events = {"user_joined_vc": true,
					"user_left_vc": true,
					"user_changed_vc_state": true};
		if(!allowed_events[name] || data.channel_id !== this.channel.data.id)
			return;

		switch(name){
			case "user_joined_vc":
				Sound.play("/src/lib/assets/sounds/vc_join.ogg");
				break;
			case "user_left_vc":
				Sound.play("/src/lib/assets/sounds/vc_leave.ogg");
				break;
			case "user_changed_vc_state":
				if(data.video === VCSocket.VideoState.Screen)
					Sound.play("/src/lib/assets/sounds/vc_video_enable.ogg");
				if(data.id === this.user_id){
					if(data.mute === VCSocket.AudioState.BySelf || data.deaf === VCSocket.AudioState.BySelf)
						Sound.play("/src/lib/assets/sounds/vc_audio_mute.ogg");
					else if(data.mute === VCSocket.AudioState.None || data.deaf === VCSocket.AudioState.None)
						Sound.play("/src/lib/assets/sounds/vc_audio_unmute.ogg");
				}
				break;
		}
	}


	// Audio management
	static AudioState = {
		None: 0,
		BySelf: 1,
		ByAdmin: 2,

		toggled: function(state){
			if(state == VCSocket.AudioState.None)
				return VCSocket.AudioState.BySelf;
			else if(state == VCSocket.AudioState.BySelf)
				return VCSocket.AudioState.None;
			return state;
		}
	};

	mute = $state(VCSocket.AudioState.None);
	toggle_mute(){
		this.mute = VCSocket.AudioState.toggled(this.mute);
		this.audio[this.user_id].ctx_dest.stream.getTracks()[0].enabled = this.mute == VCSocket.AudioState.None;
		this.ws.send(JSON.stringify({"name": "change_state", "data": {"mute": this.mute}}));
	}
	deaf = $state(VCSocket.AudioState.None);
	toggle_deaf(){
		this.deaf = VCSocket.AudioState.toggled(this.deaf);
		for(const recv of this.rtc.getReceivers())
			if(recv.track)
				recv.track.enabled = this.deaf == VCSocket.AudioState.None;
		this.ws.send(JSON.stringify({"name": "change_state", "data": {"deaf": this.deaf}}));
	}

	// Video management
	
	static VideoState = {
		None: -1,
		Disabled: 0,
		Screen: 1
	};
	
	video_state = $state(VCSocket.VideoState.Disabled);
	pending_video_state = $state(VCSocket.VideoState.None);
	is_video_enabled = $derived(this.video_state > VCSocket.VideoState);

	video_send_track;
	async set_video_state(s){
		if(s === this.video_state)
			return;

		switch(s){
			case VCSocket.VideoState.Disabled:
				this.ws.send(JSON.stringify({name: "disable_video"}));
				break;
			case VCSocket.VideoState.Screen:
				try {
					const media = await navigator.mediaDevices.getDisplayMedia();
					this.video_send_track = media.getTracks()[0];
					this.video_send_track.onended = () => {
						if(this.ws.readyState === WebSocket.OPEN)
							this.set_video_state(VCSocket.VideoState.Disabled);
					}
					this.ws.send(JSON.stringify({
						name: "enable_video",
						data: {
							codec: (this.get_video_codecs()["video/VP8"] ? "VP8" : "H264")
						}
					}));
				} catch {
					// Dont change video state if screensharing was denied
					return;
				}
				break;
		}

		this.pending_video_state = s;
	}

	watched_video = $state();
	watch_video(user_id){
		this.watched_video = this.video[user_id].track;
	}
	unwatch_video(){
		this.watched_video = undefined;
	}


	// Helper methods

	get_video_codecs(){
		const codecs = {};
		for(const c of RTCRtpReceiver.getCapabilities("video").codecs)
			codecs[c.mimeType] = true;
		return codecs;
	}

	async handle_offer(offer){
		if(!this.rtc){
			this.rtc = new RTCPeerConnection({bundlePolicy: "max-bundle"});

			// Report state changes
			this.rtc.onconnectionstatechange = () => {
				switch(this.rtc.connectionState){
					case "connecting":
						this.set_state("ICE is connecting", "--clr_text_warning");
						break;
					case "disconnected":
						this.set_state("ICE is disconnected", "--clr_text_error");
						break;
					case "failed":
						this.set_state("ICE has failed", "--clr_text_error");
						break;
					case "connected":
						this.set_state("Connected", "--clr_text_success");
						break;
				}
			};
			this.rtc.onsignalingstatechange = () => {
				switch(this.rtc.signalingState){
					case "have-local-offer":
						this.set_state("RTC has a local offer", "--clr_text_warning");
						break;
					case "have-remote-offer":
						this.set_state("RTC has a remote offer", "--clr_text_warning");
						break;
					case "stable":
						this.set_state("Connected", "--clr_text_success");
						break;
				}
			};

			// Handle new tracks added to connection
			this.rtc.ontrack = async (e) => {
				const id = e.transceiver.mid;

				e.track.enabled = this.deaf == VCSocket.AudioState.None;

				let track;
				switch(e.track.kind){
					case "audio":
						track = this.tracks[id] = new VCAudioTrack();
						break;
					case "video":
						track = this.tracks[id] = new VCVideoTrack();
						break;
					default:
						console.warn(`Unsupported track kind '${e.track.kind}'`);
						break;
				
				}
				await track.init(e.track);
				console.log("added track", track);
			};

			// Handle local RTC being ready to send counteroffer
			this.rtc.onsignalingstatechange = (state) => {
				if(this.rtc.signalingState === "stable"){
					this.ws.send(JSON.stringify({"name": "offer", "data": this.rtc.localDescription}));
					console.log("sent counteroffer");
				}
			};

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
				const new_track = media.getTracks()[0];
				
				let track = this.audio[this.user_id] = this.tracks[-1] = new VCAudioTrack();
				track.user_id = this.user_id;
				await track.init(new_track, true);
				this.rtc.addTrack(track.ctx_dest.stream.getTracks()[0], track.ctx_dest.stream);
			} catch {
			}

			this.rtc_init = true;
			this.state.text = "Initializing RTC";
		} else if(!this.rtc_init) // duplicate offer
			return;

		console.log("got offer", offer);
		await this.rtc.setRemoteDescription(offer);
		this.parse_sdp_user_ids(offer.sdp);

		switch(this.pending_video_state){
			case VCSocket.VideoState.Disabled:
				for(const tr of this.rtc.getTransceivers())
					if(tr.mid === "my_video"){
						tr.sender.track.stop();
						await tr.sender.replaceTrack(null);

						this.video_send_track = undefined;
						this.video_state = VCSocket.VideoState.Disabled;
						this.pending_video_state = VCSocket.VideoState.None;
						break;
					}
				break;
			case VCSocket.VideoState.Screen:
				for(const tr of this.rtc.getTransceivers())
					if(tr.mid === "my_video" && !tr.sender.track){
						// Manually add video send track
						tr.sender.replaceTrack(this.video_send_track);
						// Manual corrections are needed because RTCPeerConnection thinks that
						// video tracks are bidirectional, even though they are sendonly
						tr.direction = "sendonly";

						this.video_state = VCSocket.VideoState.Screen;
						this.pending_video_state = VCSocket.VideoState.None;
						break;
					}
				break;
		}

		const answer = await this.rtc.createAnswer();
		await this.rtc.setLocalDescription(answer);
		console.log("set counteroffer", answer);
	}

	parse_sdp_user_ids(sdp){
		this.audio = {[this.user_id]: this.tracks[-1]};
		this.video = {};
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

					this.tracks[mid].user_id = user_id;
					if(type.startsWith("video")){
						this.video[user_id] = this.tracks[mid];
						if(this.tracks[mid].track.id === this.watched_video?.id)
							remove_watch = false;
					}
					else if(type.startsWith("audio"))
						this.audio[user_id] = this.tracks[mid];

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
			this.unwatch_video();
	}
}
