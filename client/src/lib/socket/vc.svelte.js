import {PUBLIC_BASE_SOCKET_VC} from '$env/static/public';
import Channel from '$lib/rest/channel.js';

class VCTrack {
	user_id;

	media; track;

	volume = $state(1);

	analyser; analyser_src;
	amplitude = $state(0);
	add_analyser(src){
		if(!src)
			src = this.media.srcObject;
		this.ctx = new AudioContext();
		this.analyser = this.ctx.createAnalyser();
		this.analyser_src = this.ctx.createMediaStreamSource(src);
		this.analyser.smoothingTimeConstant = 0.1;
		this.analyser_src.connect(this.analyser);
	}
};

export default class VCSocket {
	user_self_id;
	channel;

	ws;
	rtc; rtc_init = false;

	my_audio_track;

	tracks = {};
	user_id_to_audio = $state({});
	user_id_to_video = $state({});

	static _sockets = [];
	static track_volume_intv = setInterval(() => {
		for(let i = 0; i < VCSocket._sockets.length; ++i){
			if(!VCSocket._sockets[i].deref()){
				VCSocket._sockets.splice(i, 1);
				--i;
				continue;
			}

			let sock = VCSocket._sockets[i].deref();
			for(const j in sock.tracks){
				let track = sock.tracks[j];
				if(!track.analyser)
					return;

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


	constructor(user_self_id, server_id, channel_id,
			onclose, onerror){
		this.user_self_id = user_self_id;
		this.channel = Channel.get(server_id, channel_id);

		this.ws = new WebSocket(PUBLIC_BASE_SOCKET_VC + "?channel=" + channel_id);
		this.ws.onclose = (e) => {
			if(this.rtc)
				this.rtc.close();
			// close all streams to make socket GC properly and prevent audio duplication
			for(const track of Object.values(this.tracks))
				if(track.media)
					track.media.srcObject = null;
			console.log("closed");
			onclose(e);
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

		VCSocket._sockets.push(new WeakRef(this));
	}

	end_call(){
		this.ws.close();
	}

	is_muted = $state(false);
	toggle_mute(){
		this.my_audio_track.enabled = !this.my_audio_track.enabled;
		this.is_muted = !this.my_audio_track.enabled;

		this.ws.send(JSON.stringify({"name": "change_state", "data": {"mute": this.is_muted ? 1 : 0}}));
	}
	is_deaf = $state(false);
	toggle_deaf(){
		this.is_deaf = !this.is_deaf;
		for(const recv of this.rtc.getReceivers())
			if(recv.track)
				recv.track.enabled = !this.is_deaf;

		this.ws.send(JSON.stringify({"name": "change_state", "data": {"deaf": this.is_deaf ? 1 : 0}}));
	}

	get_volume(user_id, volume){
		return this.user_id_to_audio[user_id].volume;
	}
	set_volume(user_id, volume){
		this.user_id_to_audio[user_id].volume = volume;
		this.user_id_to_audio[user_id].media.volume = volume;
	}


	static VideoState = {
		None: 0,
		Disabled: 1,
		Screen: 2
	};
	video_state = $state(VCSocket.VideoState.Disabled);
	video_state_queue = $state(VCSocket.VideoState.None);
	set_video_state(s){
		this.video_state_queue = s;

		switch(s){
			case VCSocket.VideoState.Disabled:
				this.ws.send(JSON.stringify({
					"name": "disable_video"
				}));
				break;
			case VCSocket.VideoState.Screen:
				this.ws.send(JSON.stringify({
					"name": "enable_video"
				}));
				break;
		}
	}

	watched_video = $state();
	watch_video(user_id){
		this.watched_video = this.user_id_to_video[user_id].track;
	}
	unwatch_video(){
		this.watched_video = undefined;
	}

	// Helper methods
	async handle_offer(offer){
		if(!this.rtc){
			this.rtc = new RTCPeerConnection({bundlePolicy: "max-bundle"});

			// Handle new tracks added to connection
			this.rtc.ontrack = (e) => {
				const id = e.transceiver.mid;

				e.track.enabled = !this.is_deaf;

				this.tracks[id] = new VCTrack();
				let track = this.tracks[id];
				switch(e.track.kind){
					case "audio":
						track.media = new Audio();
						track.media.srcObject = new MediaStream([e.track]);
						track.add_analyser();
						track.media.play();
						break;
					case "video":
						track.track = e.track;
						break;
					default:
						console.warn(`Unsupported track kind '${e.track.kind}'`);
						break;
				}
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
			const media = await navigator.mediaDevices.getUserMedia({audio: true});
			const new_track = media.getTracks()[0];
			this.rtc.addTrack(new_track, media);
			this.my_audio_track = new_track;
				
			this.user_id_to_audio[this.user_self_id] = this.tracks[-1] = new VCTrack();
			let track = this.tracks[-1];
			track.user_id = this.user_self_id;
			track.add_analyser(new MediaStream([new_track]));

			this.rtc_init = true;
		} else if(!this.rtc_init) // duplicate offer
			return;

		console.log("got offer", offer);
		await this.rtc.setRemoteDescription(offer);
		this.parse_sdp_user_ids(offer.sdp);

		// Add video send track
		switch(this.video_state_queue){
			case VCSocket.VideoState.Disabled:
				for(const tr of this.rtc.getTransceivers())
					if(tr.mid === "my_video"){
						tr.sender.track.stop();
						await tr.sender.replaceTrack(null);
						break;
					}
				break;
			case VCSocket.VideoState.Screen:
				for(const tr of this.rtc.getTransceivers())
					if(tr.mid === "my_video" && !tr.sender.track){
						let media;
						try {
							media = await navigator.mediaDevices.getDisplayMedia();
						}
						catch {
							this.video_state_queue = VCSocket.VideoState.None;
							this.ws.send(JSON.stringify({
								"name": "disable_video"
							}));
							break;
						}
						const new_track = media.getTracks()[0];
						new_track.onended = () => this.set_video_state(VCSocket.VideoState.Disabled);

						tr.sender.replaceTrack(new_track);
						// Manual corrections are needed because RTCPeerConnection thinks that
						// video tracks are bidirectional, even though they are sendonly
						tr.direction = "sendonly";
						break;
					}
				break;
		}
		if(this.video_state_queue !== VCSocket.VideoState.None)
			this.video_state = this.video_state_queue;
		this.video_state_queue = VCSocket.VideoState.None;

		const answer = await this.rtc.createAnswer();
		await this.rtc.setLocalDescription(answer);
		console.log("set counteroffer", answer);
	}

	parse_sdp_user_ids(sdp){
		this.user_id_to_audio = {[this.user_self_id]: this.tracks[-1]};
		this.user_id_to_video = {};
		let remove_watch = true;
		console.log("watch", this.watched_video);

		// присутствуют атрибуты mid и user_id -> дорожка используется
		let lines = sdp.split('\n');
		let type = null;
		let mid = null;
		let user_id = null;
		for(let i = 0; i < lines.length; ++i){
			let l = lines[i];
			if(l[0] == 'm' || i == lines.length - 1){
				if(mid !== null && user_id !== null && type !== null){
					mid = Number(mid);
					user_id = parseInt(user_id);
					this.tracks[mid].user_id = user_id;
					if(type.startsWith("video")){
						this.user_id_to_video[user_id] = this.tracks[mid];
						if(this.tracks[mid].track.id === this.watched_video?.id)
							remove_watch = false;
					}
					else if(type.startsWith("audio"))
						this.user_id_to_audio[user_id] = this.tracks[mid];
					mid = null;
					user_id = null;
				}
				if(l[0] == 'm')
					type = l.substring(2);
			} else if(l.startsWith('a=user:'))
				user_id = l.substring(7);
			else if(l.startsWith('a=mid:'))
				mid = l.substring(6);
		}

		if(remove_watch)
			this.watched_video = undefined;
	}
};
