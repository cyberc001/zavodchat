import Rest from '$lib/rest.js';
import Channel from '$lib/rest/channel.js';
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
		for(const _channel_id of Object.keys(Notifications.channels_todelete)){
			const channel_id = parseInt(_channel_id);
			Rest.delete("", "notifications/channels/" + channel_id,
			() => {
				let unread_messages;
				if(Channel.channel_cache.has_state(channel_id)){
					const ch = Channel.channel_cache.get_state(channel_id);
					if(ch.data.unread_messages)
						unread_messages = ch.data.unread_messages;
					delete ch.data.unread_messages;
				}
				// TODO update server list
	
				if(DM.channel_range_cache.has_state(0)){
					const ch = DM.channel_range_cache.find(0, (x) => x.id === channel_id);
					if(ch){
						if(ch.unread_messages)
							unread_messages = ch.unread_messages;
						delete ch.unread_messages;
						DM.channel_range_cache.update(0, ch);
					}
				}

				if(unread_messages && Notifications.notification_cache.has_state(0))
					Notifications.notification_cache.get_state(0).data.dm_notifications -= unread_messages;
			});
		}
		Notifications.channels_todelete = {};
	}, 1000);

	static remove_channel(channel_id){
		Notifications.channels_todelete[channel_id] = true;
	}
}
