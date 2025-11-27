import Rest from "$lib/rest";

export default class {
	static login(username, password, _then, _catch){
		Rest.post("Auth.auth", "auth", {"username": username, "password": password}, _then, _catch);
	}
	static register(username, displayname, password, _then, _catch){
		Rest.post("Auth.register", "register", {"username": username, "displayname": displayname, "password": password}, _then, _catch);
	}
}
