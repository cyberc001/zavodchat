import Denoiser from '/src/lib/socket/vc_mic_denoiser.js';

class VCMicProcessor extends AudioWorkletProcessor {
	constructor(options){
		super();
		console.log("constructing");
		Denoiser().then((res) => this.denoiser = res);
	}

	process(inputs, outputs){
		if(!this.denoiser){
			// TODO return unprocessed audio
			return true;
		}

		for(let i = 0; i < inputs.length; ++i){
			const input = inputs[i];
			for(let c = 0; c < input.length; ++c){
				const ptr = this.denoiser._malloc(input[c].length * 4);
				this.denoiser.HEAPF32.set(input[c], ptr / 4);

				this.denoiser._process(ptr, input[c].length);
				let buf = this.denoiser.HEAPF32.subarray(ptr / 4, ptr / 4 + input[c].length);

				outputs[i][c].set(buf);
				this.denoiser._free(ptr);
			}
		}

		return true;
	}
}

registerProcessor("vc_mic_processor", VCMicProcessor);
