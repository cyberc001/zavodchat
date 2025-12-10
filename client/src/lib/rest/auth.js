import Rest from "$lib/rest";

export default class {
	static login(username, password, _then, _catch){
		Rest.post("auth", {"username": username, "password": password}, _then, _catch);
	}

	static register(username, displayname, password, _then, _catch){
		Rest.post("register", {"username": username, "displayname": displayname, "password": password}, _then, _catch);
	}
	static change_user_data(data, _then, _catch){
		let fd = new FormData();
		const allowed_properties = ["displayname", "username", "password", "avatar"];
		for(const key of allowed_properties)
			if(data.hasOwnProperty(key))
				fd.append(key, data[key]);
		Rest.put("register", fd, _then, _catch);
	}
}
