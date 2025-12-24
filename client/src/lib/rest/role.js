import Rest from "$lib/rest";
import Util from "$lib/util";
import ListCache from "$lib/cache/list.svelte.js";

export default class Role {
	static role_list_cache = new ListCache();

	static update_cache(server_id, role_id, data){
		let state = Role.role_list_cache.get_state(server_id);
		let idx = state.findIndex((x) => x.id === role_id);
		if(idx !== -1){
			if(typeof data.next_role_id !== "undefined"){
				let role_data = state[idx];
				state.splice(idx, 1);
				let idx2 = data.next_role_id === -1 ? 0 : state.findIndex((x) => x.id === data.next_role_id) + 1;
				state.splice(idx2, 0, role_data);
			}
		}
	}


	static get_list(server_id, _catch){
		return Role.role_list_cache.get_state(server_id, (cache, id) => {
			Rest.get(Rest.get_route_sr(server_id, ""),
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}

	static change(server_id, role_id, data, _then, _catch){
		return Rest.put(Rest.get_route_sr(server_id, role_id),
					Util.form_data_from_object(data, ["next_role_id"]),
					(res) => _then(res.data), _catch);
	}

	static get_style(role){
		return typeof role !== "undefined" ? `background: #${role.color.toString(16).padStart(6, "0")}` : "";
	}

	static get_user_roles(user, server_id){
		if(!user || !user.roles)
			return [];

		let user_roles = [];
		for(const rol of Role.get_list(server_id)){
			const role_id = user.roles.find((x) => x === rol.id);
			if(typeof role_id !== "undefined")
				user_roles.push(rol);
		}
		return user_roles;
	};
	// Assuming role_list are ordered from highest to lowest role
	static get_username_style(role_list){
		return (typeof role_list !== "undefined" && role_list.length > 0) ? `color: #${role_list[0].color.toString(16).padStart(6, "0")}` : "";
	}
}
