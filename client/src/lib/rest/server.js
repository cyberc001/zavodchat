import {PUBLIC_BASE_REST} from '$env/static/public';
import Rest from '$lib/rest.js';
import Util from '$lib/util';
import {IDCache} from '$lib/cache/id.svelte.js';
import {ListCache} from '$lib/cache/list.svelte.js';

export default class Server {
	static server_cache = new IDCache();
	static server_list_cache = new ListCache();

	static update_cache(server_id, data){
		if(Server.server_cache.has_state(server_id)){
			let server_data = Server.server_cache.get_state(server_id);
			for(const f in data)
				server_data.data[f] = data[f];
		}
		let server_list_data = Server.server_list_cache.get_state(0).data.find((x) => x.id === server_id);
		if(server_list_data)
			for(const f in data)
				server_list_data[f] = data[f];
	}
	static remove_cache(server_id){
		Server.server_cache.remove_state(server_id);

		let list_state = Server.server_list_cache.get_state(0);
		let server_list_idx = list_state.data.findIndex((x) => x.id === server_id);
		if(server_list_idx !== -1)
			list_state.data.splice(server_list_idx, 1);
	}

	static get(server_id, _catch){
		return Server.server_cache.get_state(server_id, (cache, id) => {
			Rest.get("servers/" + server_id,
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static create(data, _then, _catch){
		Rest.post(`Creating server ${data.name}`, "servers/",
				Util.form_data_from_object(data, ["name", "avatar"]),
				(res) => _then(res.data), _catch);
	}
	static change(server_id, data, _then, _catch){
		Rest.put("Changing server", "servers/" + server_id,
			Util.form_data_from_object(data, ["name", "owner_id", "avatar"]),
			(res) => _then(res.data), _catch);
	}
	static delete(server_id, _then, _catch){
		Rest.delete("Deleting server", "servers/" + server_id,
			_then, _catch);
	}

	static get_list(_catch){
		return Server.server_list_cache.get_state(0, (cache, id) => {
			Rest.get("servers/",
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static get_list_nocache(_then, _catch){ // version for login.svelte to test token for validity
		Rest.get("servers/", (res) => _then(res.data), _catch);
	}

	static get_avatar_path(srv){
		if(typeof srv.avatar === "undefined")
			return "/src/lib/assets/default_avatar.png";
		if(srv.avatar.startsWith("data:image"))
			return srv.avatar;
		return PUBLIC_BASE_REST + "files/avatar/server/" + srv.avatar;
	}
}
