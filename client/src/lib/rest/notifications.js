import Rest from '$lib/rest.js';
import User from '$lib/rest/user.svelte.js';
import Server from '$lib/rest/server.js';
import Channel from '$lib/rest/channel.js';
import Role from '$lib/rest/role.js';
import DM from '$lib/rest/dm.js';
import {IDCache} from '$lib/cache/id.svelte.js';

export default class Notifications {
	static notification_cache = new IDCache();

	static get(_catch){
		return Notifications.notification_cache.get_state(0,
			(cache, id) => {
				Rest.get("notifications",
					(res) => cache.set_state(id, res.data),
					_catch);
		});
	}

	static channels_todelete = {};
	static __remove_channel_intv = setInterval(() => {
		for(const _ids of Object.keys(Notifications.channels_todelete)){
			const ids = _ids.split("_");
			const server_id = parseInt(ids[0]);
			const channel_id = parseInt(ids[1]);

			Rest.delete("", "notifications/channels/" + channel_id,
			() => {
				if(DM.channel_range_cache.has_state(0)){
					let ch = DM.channel_range_cache.find(0, (x) => x.id === channel_id);
					if(ch){
						let del_amt = ch.notifications;
						delete ch.notifications;
						DM.channel_range_cache.update(0, ch);

						if(del_amt && Notifications.notification_cache.has_state(0))
							Notifications.notification_cache.get_state(0).data.dm_notifications -= del_amt;
					}
				}

				let del_amt;
				if(Channel.channel_cache.has_state(channel_id)){
					const ch = Channel.channel_cache.get_state(channel_id).data;
					del_amt = ch.notifications;
					delete ch.nofitications;
				}
				if(Channel.channel_list_cache.has_state(server_id)){
					const channels = Channel.channel_list_cache.get_state(server_id).data;
					const ch = channels.find((x) => x.id === channel_id);
					if(ch){
						del_amt = ch.notifications;
						delete ch.notifications;
					}

					// If server doesn't have any more unread channels, remove notifications
					if(!channels.some((x) => typeof(x.notifications) !== "undefined")){
						Server.update_cache(server_id, {"notifications": undefined});
						del_amt = undefined;
					}
				}

				if(del_amt){
					if(Server.server_cache.has_state(server_id))
						Server.server_cache.get_state(server_id).data.notifications -= del_amt;
					if(Server.server_list_cache.has_state(0)){
						const server_list_data = Server.server_list_cache.get_state(0).data.find((x) => x.id === server_id);
						if(server_list_data)
							server_list_data.data.notifications -= del_amt;
					}
				}
			});
		}
		Notifications.channels_todelete = {};
	}, 1000);

	static remove_channel(server_id, channel_id){
		Notifications.channels_todelete[`${server_id}_${channel_id}`] = true;
	}


	static MentionTypes = {
		User: 1,
		Role: 2,
		Everyone: 3
	};

	static am_i_mentioned(mentions, server_id, _then){
		const user = User.get(-1);
		user.notify_on_load(() => {
			let self_user;
			const user_loaded = () => _then(mentions.some((x) => {
				switch(x.type){
					case Notifications.MentionTypes.User: return x.id === self_user.data.id;
					case Notifications.MentionTypes.Role: return typeof(self_user.data.roles) !== "undefined" &&
											self_user.data.roles.some((y) => y === x.id);
					case Notifications.MentionTypes.Everyone: return true;
				}
				return false;
			}));

			if(typeof(server_id) === "undefined"){
				self_user = user;
				user_loaded();
			}
			else {
				self_user = User.get_server(server_id, user.data.id);
				self_user.notify_on_load(user_loaded);
			}
		});
	}
}
