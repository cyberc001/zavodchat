export default class Sound {
	static _sounds = {};

	static play(url, loop){
		if(!Sound._sounds[url])
			Sound._sounds[url] = new Audio(url);
		Sound._sounds[url].currentTime = 0;
		Sound._sounds[url].play();
		Sound._sounds[url].loop = !!loop;
		return Sound._sounds[url];
	}

	static _current_ringtone;
	static stop_ringtone(){
		if(Sound._current_ringtone){
			Sound._current_ringtone.pause();
			Sound._current_ringtone.currentTime = 0;
		}
	}
	static play_ringtone(url){
		Sound.stop_ringtone();
		Sound._current_ringtone = Sound.play(url, true);
	}
}
