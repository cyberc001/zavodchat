import Rest from '$lib/rest.js';
import {ListCache} from '$lib/cache/list.svelte.js';
import User from '$lib/rest/user.svelte.js';

export default class Friends {
	static friend_cache = new ListCache();
	static in_requests_cache = new ListCache();
	static out_requests_cache = new ListCache();

	static get(_catch){
		return Friends.friend_cache.get_state(0, (cache, id) => {
			Rest.get("friends",
				(res) => {
					for(let i = 0; i < res.data.length; ++i){
						res.data[i].user = User.get(res.data[i].id, _catch);
						res.data[i].user.data.id = res.data[i].id; // set id even if User is not cached
					}
					cache.set_state(id, res.data);
				},
				_catch);
		});
	}
	static get_requests(incoming, _catch){
		const cache = incoming ? Friends.in_requests_cache : Friends.out_requests_cache;
		return cache.get_state(0, (cache, id) => {
			Rest.get("friend_requests",
				(res) => {
					for(let i = 0; i < res.data.length; ++i){
						res.data[i] = User.get(res.data[i], _catch);
						res.data[i].data.id = res.data[i]; // set id even if User is not cached
					}
					cache.set_state(id, res.data);
				},
				_catch,
				"incoming", incoming ? 1 : 0);
		});
	}

	static accept_or_create_request(user_id, _then, _catch){
		Rest.post("Accepting or creating a friend request", "/friends/" + user_id, undefined,
				(res) => {
					if(res.status === 200){
						if(res.data === "Friend request was sent")
							Friends.out_requests_cache.add_user_id_to_list(0, user_id);
						else {
							Friends.in_requests_cache.remove_from_list(0, user_id);
							Friends.friend_cache.add_user_id_to_list(0, user_id);
						}
					}
					if(_then)
						_then();
				},
				_catch);
	}
	static deny_request(user_id, _then, _catch){
		Rest.delete("Denying friend request", "/friends/" + user_id,
				(res) => {
					if(res.status === 200){
						if(res.data === "Cancelled friend request")
							Friends.out_requests_cache.remove_from_list(0, user_id);
						else
							Friends.in_requests_cache.remove_from_list(0, user_id);
					}
					if(_then)
						_then();
				},
				_catch);
	}
	static remove_friend(user_id, _then, _catch){
		Rest.delete("Removing friend", "/friends/" + user_id,
				(res) => {
					if(res.status === 200)
						Friends.friend_cache.remove_from_list(0, user_id);
					if(_then)
						_then();
				},
				_catch);
	}

}
