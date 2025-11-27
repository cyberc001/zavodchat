import Rest from "$lib/rest";

export default class {
	static get_list(server_id, _then, _catch){
		Rest.get(Rest.get_route_scm(server_id, ""),
			(res) => _then(res.data), _catch);
	}
}
