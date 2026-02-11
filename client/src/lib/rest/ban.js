import Rest from '$lib/rest.js';
import {RangeCache} from '$lib/cache/range.svelte.js';

export default class Ban {
	static ban_range_cache = new RangeCache();

	static get_range(server_id, start_id, count, asc, _catch){
		return Ban.ban_range_cache.get_state(server_id, start_id, count,
			(tree, start_id, count, asc) => {
				Rest.get(Rest.get_route_sb(server_id, ""),
				(res) => tree.set_state(start_id, count, res.data, asc),
				_catch,
				"start_id", start_id, "count", count, "order", asc ? 1 : 0);
			}, asc, false);
	}

	static ban(server_id, user_id, expires, _then, _catch){
		Rest.post("Banning user", Rest.get_route_sb(server_id, user_id), undefined,
				_then, _catch, "expires", expires === "never" ? expires : expires.toISOString());
	}
	static change(server_id, user_id, expires, _then, _catch){
		Rest.put("Changing ban", Rest.get_route_sb(server_id, user_id), undefined,
				_then, _catch, "expires", expires === "never" ? expires : expires.toISOString());
	}
	static unban(server_id, user_id, _then, _catch){
		Rest.delete("Unbanning user", Rest.get_route_sb(server_id, user_id), _then, _catch);
	}
}
