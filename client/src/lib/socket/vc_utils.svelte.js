import Preferences from '$lib/rest/preferences.svelte.js';

export class VCTrack {
	user_id;
	track; media;
	volume = $state(1);
	
	set_volume(vol){
		if(this.media){
			this.volume = vol;
			this.media.volume = vol;
		}
	}

	muted = false;
	blocked = false;
	set_blocked(is_blocked){
		this.blocked = is_blocked;
		this.set_muted(this.muted);
	}
	set_muted(is_muted){
		this.muted = is_muted;
		if(this.track)
			this.track.enabled = !this.muted && !this.blocked;
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

export class VCAudioTrack extends VCTrack {
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
				await this.ctx.audioWorklet.addModule( "/js/vc_mic_processor.js");
				this.processor = new AudioWorkletNode(this.ctx, "vc_mic_processor");
				this.ctx_src.connect(this.processor).connect(this.ctx_dest);
				this.processor.connect(this.analyser);
			} else {
				this.ctx_src.connect(this.ctx_dest);
				this.ctx_src.connect(this.analyser);
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
				if(track.ctx_dest && !track.ctx_dest.stream.getTracks()[0].enabled){
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
export class VCVideoTrack extends VCTrack {
}

export class AudioState {
	static None = 0;
	static BySelf = 1;
	static ByAdmin = 2;
	
	static toggled(state){
		if(state == AudioState.None)
			return AudioState.BySelf;
		else if(state == AudioState.BySelf)
			return AudioState.None;
		return state;
	}
};
export class VideoState {
	static None = -1;
	static Disabled = 0;
	static Screen = 1;
};

