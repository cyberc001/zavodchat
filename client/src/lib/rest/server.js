let route = "servers";
import Rest from "$lib/rest";
import ListCache from "$lib/cache/list.svelte.js";

export default class Server {
	static server_cache = new ListCache();

	static get_list(_catch){
		return Server.server_cache.get_state(0, (cache, id) => {
			Rest.get(Rest.get_route_scm(""),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static get_list_nocache(_then, _catch){ // version for login.svelte to test token for validity
		Rest.get(Rest.get_route_scm(""), (res) => _then(res.data), _catch);	
	}

	static get_avatar_path(srv){
		return Rest.get_base_url() + "files/avatar/server/" + srv.avatar;
	}
}
