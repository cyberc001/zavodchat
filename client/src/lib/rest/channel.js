import Rest from "$lib/rest.js";
import ListCache from "$lib/cache/list.svelte.js";

export default class Channel {
	static channel_cache = new ListCache();

	static get_list(server_id, _catch){
		return Channel.channel_cache.get_state(server_id, (cache, id) => {
			Rest.get(Rest.get_route_scm(server_id, ""),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
}
