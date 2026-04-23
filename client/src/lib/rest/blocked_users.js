import Rest from '$lib/rest.js';
import {ListCache} from '$lib/cache/list.svelte.js';
import Friends from '$lib/rest/friends.js';
import User from '$lib/rest/user.svelte.js';
import Channel from '$lib/rest/channel.js';
import Message from '$lib/rest/message.js';

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

						User.user_range_cache.reset();
						for(const server_id of Object.keys(User.user_server_range_cache.cache))
							User.user_server_range_cache.remove(server_id, user_id);

						for(const channel_id of Object.keys(Channel.channel_cache.cache))
							Channel.channel_cache.remove_state(channel_id);
						for(const server_id of Object.keys(Channel.channel_list_cache.cache)){
							const list = Channel.channel_list_cache.get_state(server_id);
							for(const ch of list.data){
								if(typeof(ch.vc_users) !== "undefined")
									delete ch.vc_users[user_id];
							}
						}

						for(const server_id of Object.keys(Message.message_range_cache.cache))
							Message.message_range_cache.remove_that(server_id, (x) => x.author_id === user_id);
					}
					if(_then)
						_then();
				},
				_catch);
	}
	static unblock_user(user_id, _then, _catch){
		Rest.delete("Unblocking user", "/blocked_users/" + user_id,
				(res) => {
					if(res.status === 200){
						BlockedUsers.blocked_users_cache.remove_from_list(0, user_id);

						User.user_range_cache.reset();
						User.user_server_range_cache.reset();

						Channel.channel_cache.reset();
						Channel.channel_list_cache.reset();

						Message.message_range_cache.reset();
					}
					if(_then)
						_then();
				},
				_catch);
	}
}
