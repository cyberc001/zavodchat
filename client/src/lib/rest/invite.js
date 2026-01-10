import Rest from '$lib/rest.svelte.js';

export default class Invite {
	static get_list_nocache(server_id, _then, _catch){
		Rest.get(Rest.get_route_si(server_id, ""),
			(res) => _then(res.data), _catch);
	}

	static get_nocache(server_id, invite_id, _then, _catch){
		Rest.get(Rest.get_route_si(server_id, invite_id),
			(res) => _then(res.data), _catch);
	}
	static create(server_id, expires, _then, _catch){
		Rest.post("Creating invite", Rest.get_route_si(server_id, ""), undefined,
				(res) => _then(res.data), _catch, "expires", expires === "never" ? expires : expires.toISOString());
	}
	static change(server_id, invite_id, expires, _then, _catch){
		Rest.put(`Changing invite ${invite_id}`, Rest.get_route_si(server_id, invite_id), undefined,
				(res) => _then(res.data), _catch, "expires", expires === "never" ? expires : expires.toISOString());
	}
	static delete(server_id, invite_id, _then, _catch){
		Rest.delete(`Removing invite ${invite_id}`, Rest.get_route_si(server_id, invite_id),
				(res) => _then(res.data), _catch);
	}

	static get_dummy_invite(){
		return {
			id: "New invite",
			expires: "never"
		};
	}
}
