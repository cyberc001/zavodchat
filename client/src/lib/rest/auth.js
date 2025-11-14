let route = "auth";
import Rest from "$lib/rest";

export default class {
	static login(username, password, _then, _catch){
		Rest.get(route, _then, _catch,
				"username", username, "password", password);
	}
	static register(username, displayname, password, _then, _catch){
		Rest.put(route, undefined, _then, _catch,
				"username", username, "displayname", displayname, "password", password);
	}
}
