import {PUBLIC_BASE_SOCKET_VC} from '$env/static/public';

export default class VCSocket {
	ws;
	rtc; rtc_init = false;

	tracks = {};

	constructor(channel_id,
			onclose, onerror) {
		this.ws = new WebSocket(PUBLIC_BASE_SOCKET_VC + "?channel=" + channel_id);
		this.ws.onclose = onclose;
		this.ws.onerror = onerror;

		this.ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			console.log("data", _data);

			switch(_data.name){
				case "offer":
					this.handle_offer(_data.data);
					break;
			}
		};
	}

	async handle_offer(offer){
		if(!this.rtc){
			this.rtc = new RTCPeerConnection({bundlePolicy: "max-bundle"});

			// Handle new tracks added to connection
			this.rtc.ontrack = (e) => {
				const id = e.transceiver.mid;

				switch(e.track.kind){
					case "audio":
						this.tracks[id] = new Audio();
						break;
					default:
						console.warn(`Unsupported track kind '${e.track.kind}'`);
				}
				console.log("adding track", id);
				this.tracks[id].srcObject = new MediaStream([e.track]);
				this.tracks[id].play();
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
			for(const track of media.getTracks()){
				this.rtc.addTrack(track, media);
				break;
			}

			this.rtc_init = true;
		} else if(!this.rtc_init) // duplicate offer
			return;

		console.log("got offer", offer);
		await this.rtc.setRemoteDescription(offer);
		const answer = await this.rtc.createAnswer();
		await this.rtc.setLocalDescription(answer);
		console.log("set counteroffer", answer);
	}
};
