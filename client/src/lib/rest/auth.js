import Rest from "$lib/rest";
import Util from "$lib/util";

export default class {
	static login(username, password, _then, _catch){
		Rest.post("auth", {"username": username, "password": password}, _then, _catch);
	}

	static register(username, displayname, password, _then, _catch){
		Rest.post("register", {"username": username, "displayname": displayname, "password": password}, _then, _catch);
	}
	static change_user_data(data, _then, _catch){
		Rest.put("register",
			Util.form_data_from_object(data, ["displayname", "username", "password", "avatar"]),
			_then, _catch);
	}
}
