import Rest from "$lib/rest";

export default class {
	static login(username, password, _then, _catch){
		Rest.post("auth", undefined, _then, _catch,
			{"Username": username, "Password": password});
	}
	static register(username, displayname, password, _then, _catch){
		Rest.put("register", undefined, _then, _catch,
			{"Username": username, "DisplayName": displayname, "Password": password});
	}
}
