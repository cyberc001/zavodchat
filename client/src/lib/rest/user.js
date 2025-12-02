import Rest from "$lib/rest.js";
import IDCache from "$lib/cache/id.svelte.js";
import RangeCache from "$lib/cache/range.svelte.js";

export default class User {
	static user_server_cache = new IDCache();
	static user_server_range_cache = new RangeCache();

	static get(user_id, _then, _catch){
		Rest.get("User.get", "users/" + user_id, (res) => _then(res.data), _catch);
	}

	static get_server(server_id, user_id, _catch){
		return User.user_server_cache.get_state([server_id, user_id], (cache, id) => {
			Rest.get("User.get_server", Rest.get_route_su(server_id, user_id),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}
	static get_server_range(server_id, start, count, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;

		return User.user_server_range_cache.get_state(server_id, start, count,
			(cache, range, start, count) => {
				Rest.get("User.get_server_range", Rest.get_route_scm(server_id) + "/users",
				(res) => cache.set_state(range, start, count, res.data),
				_catch,
				"start", start, "count", count);
			});

		/*Rest.get("User.get_server_range", Rest.get_route_scm(server_id) + "/users",
			(res) => _then(res.data), _catch,
			"start", start, "count", count);*/
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
