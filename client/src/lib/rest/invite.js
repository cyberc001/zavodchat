import Rest from '$lib/rest.svelte.js';

export default class Invite {
	static get_list_nocache(server_id, _then, _catch){
		Rest.get(Rest.get_route_si(server_id, ""),
			(res) => _then(res.data), _catch);
	}
}
