import { DataRange } from "$lib/cache/range.svelte.js";
import Message from "$lib/rest/message.js";

export default class MainSocket {
	static host;

	ws;
	constructor(onclose, onerror) {
		this.ws = new WebSocket(MainSocket.host);
		this.ws.onclose = onclose;
		this.ws.onerror = onerror;

		this.ws.onmessage = (e) => {
			const _data = JSON.parse(e.data);
			const name = _data.name;
			const data = _data.data;

			let tree;
			switch(name){
				case "message_edited":
					tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
					Message.message_range_cache.update_one_id(tree, data.id, {
						edited: data.edited,
						text: data.text,
						status: Message.Status.None
					});
					break;
				case "message_deleted":
					tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
					Message.message_range_cache.remove_one_id(tree, data.id);
					break;
				case "message_created":
					tree = Message.message_range_cache.get_tree([data.server_id, data.channel_id]);
					Message.message_range_cache.insert_last(tree, data);
					break;
			}
		};
	}
};
