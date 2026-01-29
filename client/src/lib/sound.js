export default class Sound {
	static _sounds = {};

	static play(url){
		let snd;
		if(!Sound._sounds[url])
			Sound._sounds[url] = new Audio(url);
		Sound._sounds[url].currentTime = 0;
		Sound._sounds[url].play();
	}
}
