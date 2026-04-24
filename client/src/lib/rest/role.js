import Rest from '$lib/rest.js';
import Util from '$lib/util';
import {ListCache} from '$lib/cache/list.svelte.js';
import User from '$lib/rest/user.svelte.js';

export default class Role {
	static role_list_cache = new ListCache();

	static update_cache(server_id, role_id, data){
		let state = Role.role_list_cache.get_state(server_id);
		let idx = state.findIndex((x) => x.id === role_id);
		if(idx !== -1){
			let role_data = state[idx];
			for(const key in data)
				if(key !== "next_role_id")
					role_data[key] = data[key];

			if(typeof data.next_role_id !== "undefined"){
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
	static get_list_nocache(server_id, _then, _catch){
		Rest.get(Rest.get_route_sr(server_id, ""),
			(res) => _then(res.data), _catch);
	}

	static change_list(server_id, data, _then, _catch){
		return Rest.put("Changing roles", Rest.get_route_sr(server_id, ""), data,
					(res) => _then(res.data), _catch);
	}

	static last_dummy_id = -2;
	static get_dummy_role(){
		return {
			color: "#AAAAAA",
			name: "new role",
			perms1: 0,
			id: Role.last_dummy_id--
		};
	}

	static get_background_style(role){
		return typeof role !== "undefined" ? `background: ${role.color}` : "";
	}
	static get_color_style(role){
		return typeof role !== "undefined" ? `color: ${role.color}` : "";
	}

	static get_user_roles(user, server_roles){
		if(!user || !user.roles)
			return [];

		let user_roles = [];
		for(const rol of server_roles){
			const role_id = user.roles.find((x) => x === rol.id);
			if(typeof role_id !== "undefined")
				user_roles.push(rol);
		}
		return user_roles;
	};
	// Assuming role_list are ordered from highest to lowest role
	static get_username_style(role_list){
		return (typeof role_list !== "undefined" && role_list.length > 0) ? `color: ${role_list[0].color}` : "";
	}


	static get_perm_bits(user, server, server_roles,
				set_i){
		if(server.owner_id === user.id)
			return 0x55555555;

		let bits = 0;
		for(const role of server_roles){
			if(user.roles.indexOf(role.id) === -1)
				continue;
			const p = role["perms" + set_i];

			let mask = 0;
			for(let i = 0; i < 15; ++i)
				if((bits >> (i * 2) & 0x3) === 0)
					mask |= 0x3 << (i * 2);
			bits |= p & mask;
		}
		return bits;
	}

	static check_perms(user, server, server_roles,
				set_i, perm_i,
				channel){
		if(server.owner_id === user.id)
			return true;
		for(const role of server_roles){
			if(user.roles.indexOf(role.id) === -1)
				continue;
			const channel_role_i = channel?.roles ? channel.roles.findIndex((x) => x.id === role.id) : -1;
			const chp = (channel_role_i > -1 ? channel.roles[channel_role_i].perms1 : 0) >> (perm_i * 2) & 0x3;
			const p = role["perms" + set_i] >> (perm_i * 2) & 0x3;
			if(chp !== 0)
				return chp === 1;
			if(p !== 0)
				return p === 1;
		}

		console.error(`Default role has a non-default permission, set ${set_i}, index ${perm_i}:\n`,
				server_roles);
	}
	static load_check_perms(server, set_i, perm_i,
					_then){
		User.get_self_server(server, (user_self) => {
			let server_roles = Role.get_list(server.data.id);
			server_roles.notify_on_load(() => {
				if(typeof(set_i) === "number")
					_then(Role.check_perms(user_self.data, server.data, server_roles.data, set_i, perm_i));
				else { // array
					for(let i = 0; i < set_i.length; ++i)
						if(Role.check_perms(user_self.data, server.data, server_roles.data, set_i[i], perm_i[i])){
							_then(true);
							return;
						}
					_then(false);
				}
			});
		});
	}

	static check_lower_role(user, other_role_id, server, server_roles){
		if(server.owner_id === user.id)
			return true;
		for(const role of server_roles){
			if(role.id === other_role_id)
				return false;
			else if(user.roles.indexOf(role.id) !== -1)
				return true;
		}
		return false;
	}
	static check_lower_user(user, other_user, server, server_roles){
		if(server.owner_id === other_user.id)
			return false;
		if(server.owner_id === user.id)
			return true;
		for(const role of server_roles){
			if(other_user.roles.indexOf(role.id) !== -1)
				return false;
			else if(user.roles.indexOf(role.id) !== -1)
				return true;
		}
		return false;
	}

	static perm_toggle_get(obj, set_i, perm_i, is_default){
		let x = (obj["perms" + set_i] >> (perm_i * 2)) & 0x3;
		if(is_default)
			return 2 - x;
		if(++x > 2)
			x = 0;
		return x;
	}
	static perm_toggle_set(x, obj, set_i, perm_i, is_default){
		if(is_default)
			x = 2 - x;
		else if(--x < 0)
			x = 2;
		obj["perms" + set_i] &= ~(0x3 << (perm_i * 2));
		obj["perms" + set_i] |= x << (perm_i * 2); 
	}
}
