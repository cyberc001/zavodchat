import Rest from '$lib/rest.js';
import {ListCache} from '$lib/cache/list.svelte.js';
import Friends from '$lib/rest/friends.js';
import User from '$lib/rest/user.svelte.js';

export default class BlockedUsers {
	static blocked_users_cache = new ListCache();

	static get(_catch){
		return BlockedUsers.blocked_users_cache.get_state(0, (cache, id) => {
			Rest.get("blocked_users",
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}

	static block_user(user_id, _then, _catch){
		Rest.post("Blocking user", "/blocked_users/" + user_id, undefined,
				(res) => {
					if(res.status === 200){
						BlockedUsers.blocked_users_cache.add_to_list(0, res.data);
						Friends.in_requests_cache.remove_from_list(0, user_id);
						Friends.friend_cache.remove_from_list(0, user_id);
					}
					if(_then)
						_then();
				},
				_catch);
	}
	static unblock_user(user_id, _then, _catch){
		Rest.delete("Unblocking user", "/blocked_users/" + user_id,
				(res) => {
					if(res.status === 200)
						BlockedUsers.blocked_users_cache.remove_from_list(0, user_id);
					if(_then)
						_then();
				},
				_catch);
	}
}
