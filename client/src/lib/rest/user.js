let route = "users";
import Rest from "$lib/rest";

export default class {
	static get(id, _then, _catch){
		Rest.get(route + "/" + id, (res) => _then(res.data), _catch);
	}
}
