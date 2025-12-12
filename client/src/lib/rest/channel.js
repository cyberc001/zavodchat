import Rest from "$lib/rest";
import Util from "$lib/util";
import IDCache from "$lib/cache/id.svelte.js";
import ListCache from "$lib/cache/list.svelte.js";

export default class Channel {
	static channel_cache = new IDCache();
	static channel_list_cache = new ListCache();

	static add_channel_to_cache(server_id, data){
		if(Channel.channel_list_cache.has_state(server_id))
			Channel.channel_list_cache.get_state(server_id).push(data);
	}
	static update_cache(server_id, channel_id, data){
		if(Channel.channel_cache.has_state([server_id, channel_id])){
			let channel_data = Channel.channel_cache.get_state([server_id, channel_id]);
			for(const f in data)
				channel_data[f] = data[f];
		}
		if(Channel.channel_list_cache.has_state(server_id)){
			let channel_list_data = Channel.channel_list_cache.cache[server_id].find((x) => x.id === channel_id);
			if(channel_list_data)
				for(const f in data)
					channel_list_data[f] = data[f];
		}
	}


	static get(server_id, channel_id, _catch){
		return Channel.channel_cache.get_state([server_id, channel_id], (cache, id) => {
			Rest.get(Rest.get_route_scm(server_id, channel_id),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static create(server_id, data, _then, _catch){
		Rest.post(Rest.get_route_scm(server_id, ""),
				Util.form_data_from_object(data, ["name", "type"]),
				(res) => _then(res.data), _catch);
	}
	static change(server_id, channel_id, data, _then, _catch){
		Rest.put(Rest.get_route_scm(server_id, channel_id),
				Util.form_data_from_object(data, ["name", "type"]),
				(res) => _then(res.data), _catch);
	}

	static get_list(server_id, _catch){
		return Channel.channel_list_cache.get_state(server_id, (cache, id) => {
			Rest.get(Rest.get_route_scm(server_id, ""),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}

	static Type = {
		Text: 0,
		Voice: 1
	}
}
