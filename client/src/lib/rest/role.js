import Rest from "$lib/rest";
import ListCache from "$lib/cache/list.svelte.js";

export default class Role {
	static role_list_cache = new ListCache();

	static get_list(server_id, _catch){
		return Role.role_list_cache.get_state(server_id, (cache, id) => {
			Rest.get("Role.get_list", Rest.get_route_scm(server_id) + "/roles",
				(res) => cache.set_state(id, res.data),
				_catch);
		});
	}

	static get_style(role){
		return typeof role !== "undefined" ? `background: #${role.color.toString(16)}` : "";
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
		return (typeof role_list !== "undefined" && role_list.length > 0) ? `color: #${role_list[0].color.toString(16)}` : "";
	}
}
