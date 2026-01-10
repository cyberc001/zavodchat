export default class Notifs {
	notifs = $state({});
	static _last_notif_id = 0;
	static _inst = new Notifs();

	static Types = {
		Ongoing: 0,
		Normal: 1,
		Error: 2
	};

	static add_notif(msg, type)
	{
		if(!msg || msg[0] === "!")
			return -1;
		Notifs._inst.notifs[Notifs._last_notif_id] = {message: msg, timestamp: Date.now(), type};
		return Notifs._last_notif_id++;
	}
	static remove_notif(i, error, msg)
	{
		if(error){
			if(msg){
				if(msg[0] === "!")
					msg = msg.slice(1);
				error = msg + " failed: " + error;
			}
			Notifs._inst.notifs[Notifs._last_notif_id++] = {message: error, timestamp: Date.now(), type: Notifs.Types.Error};
		}
		if(i !== -1)
			delete Notifs._inst.notifs[i];
	}
}
