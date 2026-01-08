import Rest from '$lib/rest.svelte.js';
import {RangeCache} from '$lib/cache/range.svelte.js';

export default class Ban {
	static ban_range_cache = new RangeCache();

	static get_range(server_id, start, count, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;

		return Ban.ban_range_cache.get_state(server_id, start, count,
			(cache, range, start, count) => {
				Rest.get(Rest.get_route_sb(server_id, ""),
				(res) => cache.set_state(range, start, count, res.data),
				_catch,
				"start", start, "count", count);
			});
	}

	static ban(server_id, user_id, expires, _then, _catch){
		Rest.post("Banning user", Rest.get_route_sb(server_id, user_id), undefined,
				_then, _catch, "expires", expires === "never" ? expires : expires.toISOString());
	}
	static unban(server_id, user_id, _then, _catch){
		Rest.delete("Unbanning user", Rest.get_route_sb(server_id, user_id), undefined,
				_then, _catch);
	}
}
