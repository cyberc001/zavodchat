let route = "servers";
import Rest from "$lib/rest";

export default class {
	static get_list(server_id, _then, _catch){
		Rest.get(route + "/" + server_id + "/channels", (res) => _then(res.data), _catch);
	}
}
