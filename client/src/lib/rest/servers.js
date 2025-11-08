let route = "servers";
import Rest from "$lib/rest";

export default class {
	static get_list(_then, _catch){
		Rest.get(route, _then, _catch);
	}
}
