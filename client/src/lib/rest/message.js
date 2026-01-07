import Rest from '$lib/rest.svelte.js';
import {RangeCache} from '$lib/cache/range.svelte.js';

export default class Message {
	static message_range_cache = new RangeCache();

	static get_range(server_id, channel_id, start, count, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;

		return Message.message_range_cache.get_state([server_id, channel_id], start, count,
			(cache, range, start, count) => {
				Rest.get(Rest.get_route_scm(server_id, channel_id) + "/messages",
				(res) => cache.set_state(range, start, count, res.data),
				_catch,
				"start", start, "count", count);
			});
	}

	static send(server_id, channel_id, text, _then, _catch){
		Rest.post("!Sending message", Rest.get_route_scm(server_id, channel_id) + "/messages", text,
			(res) => {_then(res.data)}, _catch);
	}
	static edit(server_id, channel_id, message_id, text, _then, _catch){
		Rest.put("!Editing message", Rest.get_route_scm(server_id, channel_id, message_id), text,
			_then, _catch);
	}

	static delete(server_id, channel_id, message_id, _then, _catch){
		Rest.delete("!Deleting message", Rest.get_route_scm(server_id, channel_id, message_id),
			_then, _catch);
	}

	static Status = {
		None: 0,
		Sending: 1,
		Deleting: 2,
		Editing: 3
	}
}
