import {PUBLIC_BASE_SOCKET} from '$env/static/public';

import Util from '$lib/util';
import Friends from '$lib/rest/friends.js';
import Message from '$lib/rest/message.js';
import User from '$lib/rest/user.svelte.js';
import Server from '$lib/rest/server.js';
import Channel from '$lib/rest/channel.js';
import Role from '$lib/rest/role.js';
import Ban from '$lib/rest/ban.js';
import DM from '$lib/rest/dm.js';

import {RangeCache} from '$lib/cache/range.svelte.js';

export default class MainSocket {
	ws;

	static socket_event_handlers = {
		friend_request_received: function(data) {
			Friends.in_requests_cache.add_user_id_to_list(0, data.id);
		},
		friend_request_accepted: function(data) {
			Friends.friend_cache.add_user_id_to_list(0, data.id);
			Friends.out_requests_cache.remove_from_list(0, data.id);
		},
		friend_request_denied: function(data) {
			Friends.out_requests_cache.remove_from_list(0, data.id);
		},
		friend_request_cancelled: function(data) {
			Friends.in_requests_cache.remove_from_list(0, data.id);
		},
		friend_removed: function(data) {
			Friends.friend_cache.remove_from_list(0, data.id);
		},

		message_edited: function(data) {
			data.status = Message.Status.None;
			Message.message_range_cache.update(data.channel_id, data);
			DM.channel_range_cache.update(0, {last_message: data});
		},
		message_deleted: function(data) {
			Message.message_range_cache.remove(data.channel_id, data.id);

			// Get new latest message
			const msgs = Message.get_range(data.channel_id, RangeCache.max_id, 1, false);
			msgs.notify_on_load(() => {
				const ch = DM.channel_range_cache.find(0, (x) => x.id === data.channel_id);
				if(ch){
					DM.channel_range_cache.remove(0, ch.last_message.id);
					if(msgs.data.length > 0){
						ch.last_message = msgs.data[0];
						DM.channel_range_cache.insert(0, ch);
					}
				}
			});
		},
		message_created: function(data) {
			console.log("message_created", data);
			Message.message_range_cache.insert(data.channel_id, data);
			// TODO keep a separate data structure that allows O(log N) search
			const ch = DM.channel_range_cache.find(0, (x) => x.id === data.channel_id);
			if(ch){
				DM.channel_range_cache.remove(0, ch.last_message.id);
				ch.last_message = data;
				DM.channel_range_cache.insert(0, ch);
			} else if(typeof(data.server_id) === "undefined") // potentially new DM channel
				DM.channel_range_cache.reload(0);
		},

		user_changed: function(data) {
			User.update_cache(data.id, data);
		},

		server_edited: function(data) {
			Server.update_cache(data.id, data);
		},
		server_deleted: function(data) {
			Server.server_cache.remove_state(data.id);
			let idx = Server.server_list_cache.get_state(0).data.findIndex((x) => x.id === data.id);
			if(typeof idx !== "undefined" && idx !== -1)
				Server.server_list_cache.get_state(0).data.splice(idx, 1);
		},

		channel_created: function(data) {
			Channel.add_channel_to_cache(data.server_id, data);
		},
		channel_edited: function(data) {
			Channel.update_cache(data.server_id, data.id, data);
		},
		channel_deleted: function(data) {
			Channel.channel_cache.remove_state(data.id);
			let st = Channel.channel_list_cache.get_state(data.server_id);
			let idx = st?.data.findIndex((x) => x.id === data.id);
			if(typeof idx !== "undefined" && idx !== -1)
				st.data.splice(idx, 1);
		},

		user_joined: function(data) {
			User.user_server_range_cache.reload(data.server_id);
		},

		user_kicked: function(data) {
			if(User.user_cache.has_state(-1) && User.user_cache.get_state(-1).data.id === data.id)
				Server.remove_cache(data.server_id);
			User.delete_cache_server(data.server_id, data.id);
		},
		user_banned: function(data) {
			if(User.user_cache.has_state(-1) && User.user_cache.get_state(-1).data.id === data.user.id)
				Server.remove_cache(data.server_id);
			User.delete_cache_server(data.server_id, data.user.id);
			Ban.ban_range_cache.insert(data.server_id, data);
		},
		user_unbanned: function(data) {
			Ban.ban_range_cache.remove(data.server_id, data.id);
		},
		ban_changed: function(data) {
			Ban.ban_range_cache.update(data.server_id, data);
		},

		roles_updated: function(data) {
			let list = Role.role_list_cache.get_state(data.server_id);
			if(list && !list.loading){
				list.data.splice(0, list.data.length);
				for(const rol of data.roles)
					list.data.push(rol);
			}
		},

		role_assigned: function(data) {
			let new_roles = User.get_roles(data.server_id, data.user_id);
			if(new_roles){
				new_roles.push(data.role_id);
				User.update_cache_server(data.server_id, {id: data.user_id, roles: new_roles});
			}
		},
		role_disallowed: function(data) {
			let new_roles = User.get_roles(data.server_id, data.user_id);
			if(new_roles){
				const i = new_roles.findIndex((x) => x === data.role_id);
				if(i !== -1){
					new_roles.splice(i, 1);
					User.update_cache_server(data.server_id, {id: data.user_id, roles: new_roles});
				}
			}
		},

		user_joined_vc: function(data) {
			if(typeof(data.server_id) !== "undefined"){
				if(Channel.channel_list_cache.has_state(data.server_id)){
					let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
					if(channel_list_data && channel_list_data.vc_users){
						channel_list_data.vc_users[data.id] = Util.object_from_object(data, ["id", "mute", "deaf"]);
						channel_list_data.vc_users[data.id].user = User.get_server(data.server_id, data.id);
					}
				}
			} else {
				if(Friends.friend_cache.has_state(0)){
					const friend_data = Friends.friend_cache.get_state(0).data.find((x) => x.id === data.id);
					if(friend_data){
						console.log("FRIEND STATE", friend_data.vc_users, friend_data.vc_users?.length);
						if(friend_data.vc_users)
							friend_data.vc_users.push(data);
						else
							friend_data.vc_users = [data];
						friend_data.vc_channel_id = data.channel_id;
						console.log("FRIEND STATE AFTER", friend_data.vc_users, friend_data.vc_users?.length);
					}
				}
			}
		},
		user_left_vc: function(data) {
			if(typeof(data.server_id) !== "undefined"){
				if(Channel.channel_list_cache.has_state(data.server_id)){
					let list = Channel.channel_list_cache.get_state(data.server_id);
					let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
					if(channel_list_data && channel_list_data.vc_users)
						delete channel_list_data.vc_users[data.id];
				}
			} else {
				if(Friends.friend_cache.has_state(0)){
					const friend_data = Friends.friend_cache.get_state(0).data.find((x) => x.id === data.id);
					if(friend_data?.vc_users){
						let i = friend_data.vc_users.findIndex((x) => x.id === data.id);
						if(i > -1)
							friend_data.vc_users.splice(i, 1);
					}
				}			
			}
		},
		user_changed_vc_state: function(data) {
			if(typeof(data.server_id) !== "undefined"){
				if(Channel.channel_list_cache.has_state(data.server_id)){
					let list = Channel.channel_list_cache.get_state(data.server_id);
					let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
					if(channel_list_data && channel_list_data.vc_users){
						if(typeof data.mute !== "undefined")
							channel_list_data.vc_users[data.id].mute = data.mute;
						if(typeof data.deaf !== "undefined")
							channel_list_data.vc_users[data.id].deaf = data.deaf;
						if(typeof data.video !== "undefined")
							channel_list_data.vc_users[data.id].video = data.video;
					}
				}
			}
		}
	};

	ws_ping_intv;

	constructor(onclose, onerror, onmessage) {
		this.ws = new WebSocket(PUBLIC_BASE_SOCKET);
		this.ws.onclose = (e) => {
			console.log(e);
			clearInterval(this.ws_ping_intv);
			onclose(e);
		};
		this.ws.onerror = onerror;

		this.ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			if(Object.hasOwn(MainSocket.socket_event_handlers, _data.name))
				MainSocket.socket_event_handlers[_data.name](_data.data);
			else
				console.warn("Unhandled main WebSocket event", _data);
			if(onmessage)
				onmessage(_data.name, _data.data);
		};

		this.ws_ping_intv = setInterval(() => {
			this.ws.send(JSON.stringify({name: "ping", data: ""}));
		}, 60000);
	}
};
