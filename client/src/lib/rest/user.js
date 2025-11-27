import Rest from "$lib/rest";

export default class User {
	static get(user_id, _then, _catch){
		Rest.get("User.get", "users/" + user_id, (res) => _then(res.data), _catch);
	}

	static get_server(server_id, user_id, _then, _catch){
		Rest.get("User.get_server", Rest.get_route_su(server_id, user_id), (res) => _then(res.data), _catch);
	}
	static get_server_range(server_id, start, count, _then, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;
		Rest.get("User.get_server_range", Rest.get_route_scm(server_id) + "/users",
			(res) => _then(res.data), _catch,
			"start", start, "count", count);
	}

	static dummy(){
		return {
			id: 0,
			name: "",
			status: 0,
			roles: []
		};
	}

	static get_avatar_path(usr){
		return Rest.get_base_url() + "files/avatar/user/" + usr.avatar;
	}

	static Status = {
		Offline: 0,
		Online: 1,
		Away: 2,
		DoNotDisturb: 3,

		get_style: (st) => {
			let var_name;
			switch(st){
				case User.Status.Offline:
					var_name = "clr_offline";
					break;
				case User.Status.Online:
					var_name = "clr_online";
					break;
				case User.Status.Away:
					var_name = "clr_away";
					break;
				case User.Status.DoNotDisturb:
					var_name = "clr_donotdisturb";
					break;
			}
			return typeof var_name === "undefined" ? "" : `background: var(--${var_name})`;
		}
	}
}
