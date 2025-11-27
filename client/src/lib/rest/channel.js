import Rest from "$lib/rest";

export default class {
	static get_list(server_id, _then, _catch){
		Rest.get("Channel.get_list", Rest.get_route_scm(server_id, ""),
			(res) => _then(res.data), _catch);
	}
}
