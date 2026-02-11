import {PUBLIC_BASE_REST} from '$env/static/public';
import Rest from '$lib/rest.js';
import {IDCache} from '$lib/cache/id.svelte.js';
import {RangeCache} from '$lib/cache/range.svelte.js';
import {asset} from '$app/paths';

export default class User {
	static user_cache = new IDCache();
	static user_server_cache = new IDCache();
	static user_server_range_cache = new RangeCache();

	static add_shared_server(user_id, server_id){
		let u = User.user_cache.get_state(user_id);
		if(typeof u.data.servers === "undefined")
			u.data.servers = [server_id];
		else
			u.data.servers.push(server_id);
		return u;
	}

	static update_cache(user_id, data){
		// try to update user_self
		if(user_id !== -1 && User.user_cache.has_state(-1)
			&& User.user_cache.get_state(-1).data.id === user_id){
			let user_data = User.user_cache.get_state(-1);
			for(const f in data)
				user_data.data[f] = data[f];
		}

		if(!User.user_cache.has_state(user_id))
			return;

		let user_data = User.user_cache.get_state(user_id);
		for(const f in data)
			user_data.data[f] = data[f];

		// for each server that user shared with the client
		for(const server_id of user_data.data.servers)
			User.update_cache_server(server_id, user_id, data);
	}

	static update_cache_server(server_id, data){
		if(User.user_server_cache.has_state([server_id, data.id])){
			let st = User.user_server_cache.get_state([server_id, data.id]);
			for(const f in data)
				st.data[f] = data[f];
		}
		User.user_server_range_cache.update(server_id, data);
	}
	static delete_cache_server(server_id, user_id){
		User.user_server_cache.remove_state([server_id, user_id]);
		User.user_server_range_cache.remove(server_id, user_id);
	}


	static get(user_id, _catch){
		return User.user_cache.get_state(user_id,
			(cache, id) => {
				Rest.get("users/" + user_id,
					(res) => cache.set_state(id, res.data),
					_catch);
		});
	}
	static get_nocache(user_id, _then, _catch){
		Rest.get("users/" + user_id, (res) => _then(res.data), _catch);
	}


	static get_server(server_id, user_id, _catch){
		return User.user_server_cache.get_state([server_id, user_id],
			(cache, id) => {
			Rest.get(Rest.get_route_sur(server_id, user_id),
				(res) => {
					res.data.user_ref = User.add_shared_server(user_id, server_id);
					cache.set_state(id, res.data);
				},
				(err) => {
					if(err.status === 404)
						Rest.get("users/" + user_id,
							(res) => cache.set_state(id, res.data),
							_catch);
					else
						_catch(err);
				});
		});
	}
	static get_server_range(server_id, start_id, count, asc, displayname, _catch){
		return User.user_server_range_cache.get_state(typeof displayname == "undefined" ? server_id : [server_id, displayname],
			start_id, count,
			(tree, start_id, count, asc) => {
				const res_handler = (res) => {
						for(let user of res.data)
							user.user_ref = User.add_shared_server(user.id, server_id);
						tree.set_state(start_id, count, res.data, asc);
				};

				if(typeof displayname !== "undefined")
					Rest.get(Rest.get_route_sur(server_id, ""),
						res_handler, _catch,
						"start_id", start_id, "count", count, "order", asc ? 1 : 0, "displayname", displayname);
				else
					Rest.get(Rest.get_route_sur(server_id, ""),
						res_handler, _catch,
						"start_id", start_id, "count", count, "order", asc ? 1 : 0);
			}, asc, true);
	}

	static kick(server_id, user_id, _then, _catch){
		Rest.delete("Kicking user", Rest.get_route_sur(server_id, user_id),
						_then, _catch);
	}

	static get_roles(server_id, user_id){
		if(User.user_server_cache.has_state([server_id, user_id]))
			return User.user_server_cache.get_state([server_id, user_id]).data.roles;
		else
			return User.user_server_range_cache.get_tree(server_id)?.find_by_id(user_id)?.data.roles;
	}

	static assign_role(server_id, user_id, role_id, _then, _catch){
		Rest.post("Assigning role", Rest.get_route_sur(server_id, user_id, role_id), undefined,
				(res) => _then(res.data), _catch);
	}
	static disallow_role(server_id, user_id, role_id, _then, _catch){
		Rest.delete("Disallowing role", Rest.get_route_sur(server_id, user_id, role_id),
				(res) => _then(res.data), _catch);
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
		if(typeof usr.avatar === "undefined")
			return asset("default_avatar.png");
		return PUBLIC_BASE_REST + "files/avatar/user/" + usr.avatar;
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
