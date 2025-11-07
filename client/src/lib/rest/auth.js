let route = "auth";
import rest from "$lib/rest";

export default class {
	static login(username, password, _then, _catch){
		rest.get(route, _then, _catch,
				"username", username, "password", password);
	}
	static register(username, displayname, password, _then, _catch){
		rest.put(route, _then, _catch,
				"username", username, "displayname", displayname, "password", password);
	}
}
