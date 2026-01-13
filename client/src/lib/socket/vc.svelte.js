import {PUBLIC_BASE_SOCKET_VC} from '$env/static/public';
import Channel from '$lib/rest/channel.js';

class VCTrack {
	user_id;

	media;
	analyser; analyser_src;

	amplitude = $state(0); volume = $state(1);

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
	user_id_to_track = $state({});

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
		this.ws.onclose = onclose;
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
		this.rtc.close();
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
		return this.user_id_to_track[user_id].volume;
	}
	set_volume(user_id, volume){
		this.user_id_to_track[user_id].volume = volume;
		this.user_id_to_track[user_id].media.volume = volume;
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
						break;
					default:
						console.warn(`Unsupported track kind '${e.track.kind}'`);
				}
				console.log("adding track", track);
				track.media.play();
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
			for(const new_track of media.getTracks()){
				this.rtc.addTrack(new_track, media);
				this.my_audio_track = new_track;
				
				this.user_id_to_track[this.user_self_id] = this.tracks[-1] = new VCTrack();
				let track = this.tracks[-1];
				track.user_id = this.user_self_id;
				track.add_analyser(new MediaStream([new_track]));
				break;
			}

			this.rtc_init = true;
		} else if(!this.rtc_init) // duplicate offer
			return;

		console.log("got offer", offer);
		await this.rtc.setRemoteDescription(offer);
		this.parse_sdp_user_ids(offer.sdp);

		const answer = await this.rtc.createAnswer();
		await this.rtc.setLocalDescription(answer);
		console.log("set counteroffer", answer);
	}

	parse_sdp_user_ids(sdp){
		this.user_id_to_track = {[this.user_self_id]: this.tracks[-1]};

		// присутствуют атрибуты mid и user_id -> дорожка используется
		let lines = sdp.split('\n');
		let mid = null;
		let user_id = null;
		for(let i = 0; i < lines.length; ++i){
			let l = lines[i];
			if(l[0] == 'm' || i == lines.length - 1){
				if(mid !== null && user_id !== null){
					mid = Number(mid);
					user_id = parseInt(user_id);
					this.tracks[mid].user_id = user_id;
					this.user_id_to_track[user_id] = this.tracks[mid];
					mid = null;
					user_id = null;
				}
			} else if(l.startsWith('a=user:'))
				user_id = l.substring(7);
			else if(l.startsWith('a=mid:'))
				mid = l.substring(6);
		}
	}
};
