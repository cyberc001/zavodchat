import {PUBLIC_BASE_REST} from '$env/static/public';
import Rest from '$lib/rest.js';
import Util from '$lib/util';
import {ListCache} from '$lib/cache/list.svelte.js';

export default class Emoji {
	static server_list_cache = new ListCache();

	static get_list(server_id, _catch){
		return Emoji.server_list_cache.get_state(server_id, (cache, id) => {
			Rest.get(Rest.get_route_se(server_id, ""),
				(res) => cache.set_state(id, res.data), _catch);
		});
	}

	static create(server_id, data, _then, _catch){
		Rest.post("Creating emoji", Rest.get_route_se(server_id, ""),
				Util.form_data_from_object(data),
				_then, _catch);
	}


	static change(server_id, emoji_id, data, _then, _catch){
		Rest.put("Changing emoji", Rest.get_route_se(server_id, emoji_id),
				Util.form_data_from_object(data),
				_then, _catch);
	}

	static delete(server_id, emoji_id, _then, _catch){
		Rest.delete("Removing emoji", Rest.get_route_se(server_id, emoji_id),
				_then, _catch);
	}


	static get_image_path(emoji){
		if(emoji.image.startsWith("data:image"))
			return emoji.image;
		return PUBLIC_BASE_REST + "files/emoji/" + emoji.image;
	}
}
