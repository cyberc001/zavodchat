import Denoiser from '/src/lib/socket/vc_mic_denoiser.js';

class VCMicProcessor extends AudioWorkletProcessor {
	constructor(options){
		Denoiser().then((res) => this.denoiser = res);
		super();
	}

	process(inputs, outputs){
		// Assume only one mono input
		const input = inputs[0][0];
		if(!input)
			return true;

		if(!this.denoiser){
			// return unprocessed audio
			outputs[0][0].set(input);
			return true;
		}

		const ptr = this.denoiser._get_buf();
		this.denoiser.HEAPF32.set(input, ptr / 4);

		this.denoiser._process(input.length);

		let buf = this.denoiser.HEAPF32.subarray(ptr / 4, ptr / 4 + input.length);
		outputs[0][0].set(buf);
		// handle stereo output
		if(outputs[0][1])
			outputs[0][1].set(buf);

		return true;
	}
}

registerProcessor("vc_mic_processor", VCMicProcessor);
