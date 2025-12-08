import Rest from "$lib/rest.js";
import IDCache from "$lib/cache/id.svelte.js";
import RangeCache from "$lib/cache/range.svelte.js";

export default class User {
	static user_cache = new IDCache();
	static user_server_cache = new IDCache();
	static user_server_range_cache = new RangeCache();

	static add_shared_server(user_id, server_id){
		let u = User.user_cache.get_state(user_id);
		if(!u.hasOwnProperty("servers"))
			u.servers = [server_id];
		else
			u.servers.push(server_id);
	}

	static update_cache(user_id, data){
		if(!User.user_cache.cache.hasOwnProperty(user_id))
			return;

		let user_data = User.user_cache.get_state(user_id);
		for(const f in data)
			user_data[f] = data[f];

		// for each server that user shared with the client
		for(const server_id of user_data.servers){
			for(const f in data)
				User.user_server_cache.get_state([server_id, user_id])[f] = data[f];

			let tree = User.user_server_range_cache.get_tree(server_id);
			if(tree)
				tree.update_one_id(user_id, data);
		}
	}


	static get(user_id, _catch){
		return User.user_cache.get_state(user_id,
			(cache, id) => {
				Rest.get("users/" + user_id,
					(res) => cache.set_state(id, res.data),
					_catch);
		});

		Rest.get("User.get", "users/" + user_id, (res) => _then(res.data), _catch);
	}

	static get_server(server_id, user_id, _catch){
		return User.user_server_cache.get_state([server_id, user_id],
			(cache, id) => {
			Rest.get(Rest.get_route_su(server_id, user_id),
				(res) => {
					cache.set_state(id, res.data);
					User.add_shared_server(user_id, server_id);
				},
				_catch);
		});
	}
	static get_server_range(server_id, start, count, _catch){
		if(start == -1) start = 0;
		if(count == -1) count = 50;

		return User.user_server_range_cache.get_state(server_id, start, count,
			(cache, range, start, count) => {
				Rest.get(Rest.get_route_scm(server_id) + "/users",
				(res) => {
					cache.set_state(range, start, count, res.data);
					for(const user of res.data)
						User.add_shared_server(user.id, server_id);
				},
				_catch,
				"start", start, "count", count);
			});
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
