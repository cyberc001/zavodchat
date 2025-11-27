let route = "servers";
import Rest from "$lib/rest";

export default class {
	static get_list(_then, _catch){
		Rest.get("Server.get_list", route, (res) => _then(res.data), _catch);
	}

	static get_avatar_path(srv){
		return Rest.get_base_url() + "files/avatar/server/" + srv.avatar;
	}
}
