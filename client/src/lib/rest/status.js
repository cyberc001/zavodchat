import Rest from '$lib/rest.js';

export default class Status {
	static change(_status, _then, _catch){
		Rest.put("Changing status", "/user_status",
				undefined,
				_then, _catch,
				"status", _status);
	}

	static Offline = 0;
	static Online = 1;
	static Away = 2;
	static DoNotDisturb = 3;
}
