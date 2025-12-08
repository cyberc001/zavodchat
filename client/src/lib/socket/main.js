import { DataRange } from "$lib/cache/range.svelte.js";
import Message from "$lib/rest/message.js";
import User from "$lib/rest/user.svelte.js";

export default class MainSocket {
	static host;
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

		user_status_changed: function(data) {
			User.update_cache(data.id, {status: data.status});
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
