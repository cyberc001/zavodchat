import {PUBLIC_BASE_SOCKET} from '$env/static/public';

import {DataRange} from '$lib/cache/range.svelte.js';
import Util from '$lib/util';
import Message from '$lib/rest/message.js';
import User from '$lib/rest/user.svelte.js';
import Server from '$lib/rest/server.js';
import Channel from '$lib/rest/channel.js';
import Role from '$lib/rest/role.js';
import Ban from '$lib/rest/ban.js';

export default class MainSocket {
	ws;

	static _get_new_roles(data) {
		if(User.user_server_cache.has_state([data.server_id, data.user_id]))
			return User.user_server_cache.get_state([data.server_id, data.user_id]).data.roles;
		else
			return User.user_server_range_cache.get_tree(data.server_id)?.find_by_id(data.user_id)?.data.roles;
	}

	static socket_event_handlers = {
		message_edited: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			if(tree)
				tree.update_one_id(data.id, {
					edited: data.edited,
					text: data.text,
					status: Message.Status.None
				});
		},
		message_deleted: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			if(tree)
				tree.remove_one_id(data.id);
		},
		message_created: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			if(tree)
				tree.insert_last(data);
		},

		user_changed: function(data) {
			User.update_cache(data.id, Util.object_from_object(data, ["status", "name", "avatar"]));
		},

		server_edited: function(data) {
			Server.update_cache(data.id, Util.object_from_object(data, ["name", "avatar"]));
		},
		server_deleted: function(data) {
			Server.server_cache.remove_state(data.id);
			let idx = Server.server_list_cache.get_state(0).data.findIndex((x) => x.id === data.id);
			if(typeof idx !== "undefined" && idx !== -1)
				Server.server_list_cache.get_state(0).data.splice(idx, 1);
		},

		channel_created: function(data) {
			Channel.add_channel_to_cache(data.server_id, Util.object_from_object(data, ["name", "type", "id"]));
		},
		channel_edited: function(data) {
			Channel.update_cache(data.server_id, data.id, Util.object_from_object(data, ["name", "type"]));
		},
		channel_deleted: function(data) {
			Channel.channel_cache.remove_state(data.id);
			let st = Channel.channel_list_cache.get_state(data.server_id);
			let idx = st?.data.findIndex((x) => x.id === data.id);
			if(typeof idx !== "undefined" && idx !== -1)
				st.data.splice(idx, 1);
		},

		user_kicked: function(data) {
			if(User.user_cache.has_state(-1) && User.user_cache.get_state(-1).data.id === data.id)
				Server.remove_cache(data.server_id);
			User.delete_cache_server(data.server_id, data.id);
		},
		user_banned: function(data) {
			if(User.user_cache.has_state(-1) && User.user_cache.get_state(-1).data.id === data.id)
				Server.remove_cache(data.server_id);
			User.delete_cache_server(data.server_id, data.id);

			let tree = Ban.ban_range_cache.get_tree(data.server_id);
			if(tree)
				User.get_nocache(data.id, (user_data) => {
						user_data.expires = data.expires;
						tree.insert_last(user_data);
				}, () => {});
		},
		user_unbanned: function(data) {
			let tree = Ban.ban_range_cache.get_tree(data.server_id);
			if(tree)
				tree.remove_one_id(data.id);
		},
		ban_changed: function(data) {
			let tree = Ban.ban_range_cache.get_tree(data.server_id);
			if(tree)
				tree.update_one_id(data.id, {expires: data.expires});
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
			let new_roles = MainSocket._get_new_roles(data);
			if(new_roles){
				new_roles.push(data.role_id);
				User.update_cache_server(data.server_id, data.user_id, {roles: new_roles});
			}
		},
		role_disallowed: function(data) {
			let new_roles = MainSocket._get_new_roles(data);
			if(new_roles){
				const i = new_roles.findIndex((x) => x === data.role_id);
				if(i !== -1){
					new_roles.splice(i, 1);
					User.update_cache_server(data.server_id, data.user_id, {roles: new_roles});
				}
			}
		},

		user_joined_vc: function(data) {
			if(Channel.channel_list_cache.has_state(data.server_id)){
				let list = Channel.channel_list_cache.get_state(data.server_id);
				let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
				if(channel_list_data && channel_list_data.vc_users){
					channel_list_data.vc_users[data.id] = Util.object_from_object(data, ["id", "mute", "deaf"]);
					channel_list_data.vc_users[data.id].user = User.get_server(data.server_id, data.id);
				}
			}
		},
		user_left_vc: function(data) {
			if(Channel.channel_list_cache.has_state(data.server_id)){
				let list = Channel.channel_list_cache.get_state(data.server_id);
				let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
				if(channel_list_data && channel_list_data.vc_users)
					delete channel_list_data.vc_users[data.id];
			}
		},
		user_changed_vc_state: function(data) {
			if(Channel.channel_list_cache.has_state(data.server_id)){
				let list = Channel.channel_list_cache.get_state(data.server_id);
				let channel_list_data = Channel.channel_list_cache.get_state(data.server_id).data.find((x) => x.id === data.channel_id);
				if(channel_list_data && channel_list_data.vc_users){
					channel_list_data.vc_users[data.id].mute = data.mute;
					channel_list_data.vc_users[data.id].deaf = data.deaf;
				}
			}
		}
	};

	constructor(onclose, onerror, onmessage) {
		this.ws = new WebSocket(PUBLIC_BASE_SOCKET);
		this.ws.onclose = onclose;
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
	}
};
