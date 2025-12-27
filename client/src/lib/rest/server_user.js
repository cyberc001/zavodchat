import Rest from "$lib/rest.js";

export default class ServerUser {
	static assign_role(server_id, user_id, role_id, _then, _catch){
		Rest.post(Rest.get_route_sur(server_id, user_id, role_id), undefined,
				(res) => _then(res.data), _catch);
	}
	static disallow_role(server_id, user_id, role_id, _then, _catch){
		Rest.delete(Rest.get_route_sur(server_id, user_id, role_id),
				(res) => _then(res.data), _catch);
	}
}
