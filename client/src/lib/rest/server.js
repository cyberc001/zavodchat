let route = "servers";
import Rest from "$lib/rest";
import Util from "$lib/util";
import IdCache from "$lib/cache/id.svelte.js";
import ListCache from "$lib/cache/list.svelte.js";

export default class Server {
	static server_cache = new IdCache();
	static server_list_cache = new ListCache();

	static update_cache(server_id, data){
		if(Server.server_cache.has_state(server_id)){
			let server_data = Server.server_cache.get_state(server_id);
			for(const f in data)
				server_data[f] = data[f];
		}
		let server_list_data = Server.server_list_cache.cache[0].find((x) => x.id === server_id);
		if(server_list_data)
			for(const f in data)
				server_list_data[f] = data[f];
	}

	static get(server_id, _catch){
		return Server.server_cache.get_state(server_id, (cache, id) => {
			Rest.get(Rest.get_route_scm(server_id),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static create(data, _then, _catch){
		Rest.post(Rest.get_route_scm(""),
				Util.form_data_from_object(data, ["name", "avatar"]),
				(res) => _then(res.data), _catch);
	}
	static change(server_id, data, _then, _catch){
		Rest.put(Rest.get_route_scm(server_id),
			Util.form_data_from_object(data, ["name", "owner_id", "avatar"]),
			(res) => _then(res.data), _catch);
	}
	static delete(server_id, _then, _catch){
		Rest.delete(Rest.get_route_scm(server_id),
			_then, _catch);
	}

	static get_list(_catch){
		return Server.server_list_cache.get_state(0, (cache, id) => {
			Rest.get(Rest.get_route_scm(""),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static get_list_nocache(_then, _catch){ // version for login.svelte to test token for validity
		Rest.get(Rest.get_route_scm(""), (res) => _then(res.data), _catch);
	}

	static get_avatar_path(srv){
		if(typeof srv.avatar === "undefined")
			return "/src/lib/assets/default_avatar.png";
		if(srv.avatar.startsWith("data:image"))
			return srv.avatar;
		return Rest.get_base_url() + "files/avatar/server/" + srv.avatar;
	}
}
