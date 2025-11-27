import Rest from "$lib/rest";

export default class {
	static get_range(server_id, channel_id, start, count, _then, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;
		Rest.get("Message.get_range", Rest.get_route_scm(server_id, channel_id) + "/messages",
			(res) => _then(res.data), _catch,
			"start", start, "count", count);
	}

	static send(server_id, channel_id, text, _then, _catch){
		Rest.post("Message.send", Rest.get_route_scm(server_id, channel_id) + "/messages", text,
			(res) => {_then(res.data)}, _catch);
	}
	static edit(server_id, channel_id, message_id, text, _then, _catch){
		Rest.put("Message.edit", Rest.get_route_scm(server_id, channel_id, message_id), text,
			_then, _catch);
	}

	static delete(server_id, channel_id, message_id, _then, _catch){
		Rest.delete("Message.delete", Rest.get_route_scm(server_id, channel_id, message_id),
			_then, _catch);
	}

	static Status = {
		None: 0,
		Sending: 1,
		Deleting: 2,
		Editing: 3
	}
}
