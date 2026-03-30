import Rest from '$lib/rest.js';
import {RangeCache} from '$lib/cache/range.svelte.js';

export default class DM {
	static channel_range_cache = new RangeCache((x) => x.last_message?.id, (id) => { return {last_message: {id}}; });

	static get_channels(start_id, count, asc, _catch){
		return DM.channel_range_cache.get_state(0, start_id, count,
			(tree, start_id, count, asc) => {
				Rest.get("dms",
					(res) => tree.set_state(start_id, count, res.data, asc),
					_catch,
					"start_id", start_id, "count", count, "order", asc ? 1 : 0);
			}, asc, false);
	}

	static open(user_id, _then, _catch){
		Rest.post("Opening DM channels", "dms/" + user_id, undefined,  _then, _catch);
	}
}
