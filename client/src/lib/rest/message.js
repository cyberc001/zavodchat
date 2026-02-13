import Rest from '$lib/rest.js';
import {RangeCache} from '$lib/cache/range.svelte.js';

export default class Message {
	static message_range_cache = new RangeCache();

	static get_range(channel_id, start_id, count, asc, _catch){
		return Message.message_range_cache.get_state(channel_id, start_id, count,
			(tree, start_id, count, asc) => {
				Rest.get("channels/" + channel_id + "/messages",
					(res) => tree.set_state(start_id, count, res.data, asc),
					_catch,
					"start_id", start_id, "count", count, "order", asc ? 1 : 0);
			}, asc, true);
	}
	static get_search_range(channel_id, start_id, count, asc, params, _catch){
		let cache_id = [channel_id];
		for(const key of Object.keys(params)){
			cache_id.push(key);
			cache_id.push(params[key]);
		}
		return Message.message_range_cache.get_state(cache_id,
			start_id, count,
			(tree, start_id, count, asc) => {
				Rest.post("", "channels/" + channel_id + "/messages_search", params,
					(res) => tree.set_state(start_id, count, res.data, asc),
					_catch,
					"start_id", start_id, "count", count, "order", asc ? 1 : 0);
			}, asc, true);
	}


	static send(channel_id, text, _then, _catch){
		Rest.post("!Sending message", "channels/" + channel_id + "/messages", text,
			(res) => {_then(res.data)}, _catch);
	}
	static edit(message_id, text, _then, _catch){
		Rest.put("!Editing message", "messages/" + message_id, text,
			_then, _catch);
	}

	static delete(message_id, _then, _catch){
		Rest.delete("!Deleting message", "messages/" + message_id,
			_then, _catch);
	}

	static Status = {
		None: 0,
		Sending: 1,
		Deleting: 2,
		Editing: 3
	}
}
