import { DataRange } from "$lib/cache/range.svelte.js";
import Util from "$lib/util";
import Message from "$lib/rest/message.js";
import User from "$lib/rest/user.svelte.js";
import Server from "$lib/rest/server.js";
import Channel from "$lib/rest/channel.js";

export default class MainSocket {
	static host = "wss://127.0.0.1:444";
	ws;

	static socket_event_handlers = {
		message_edited: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			tree.update_one_id(data.id, {
				edited: data.edited,
				text: data.text,
				status: Message.Status.None
			});
		},
		message_deleted: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			tree.remove_one_id(data.id);
		},
		message_created: function(data) {
			let tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
			tree.insert_last(data);
		},

		user_changed: function(data) {
			User.update_cache(data.id, Util.object_from_object(data, ["status", "name", "avatar"]));
		},

		server_edited: function(data) {
			Server.update_cache(data.id, Util.object_from_object(data, ["name", "avatar"]));
		},

		channel_created: function(data) {
			Channel.add_channel_to_cache(data.server_id, Util.object_from_object(data, ["name", "type"]));
		},
		channel_edited: function(data) {
			Channel.update_cache(data.server_id, data.id, Util.object_from_object(data, ["name", "type"]));
		}
	};

	constructor(onclose, onerror) {
		this.ws = new WebSocket(MainSocket.host);
		this.ws.onclose = onclose;
		this.ws.onerror = onerror;

		this.ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			if(Object.hasOwn(MainSocket.socket_event_handlers, _data.name))
				MainSocket.socket_event_handlers[_data.name](_data.data);
			else
				console.warn("Unhandled main WebSocket event", _data);
		};
	}
};
