import Rest from "$lib/rest";

export default class {
	static get_list(server_id, _then, _catch){
		Rest.get("Role.get_list", Rest.get_route_scm(server_id) + "/roles",
			(res) => _then(res.data), _catch);
	}

	static get_style(role){
		return typeof role !== "undefined" ? `background: #${role.color.toString(16)}` : "";
	}

	// Assuming role_list are ordered from highest to lowest role
	static get_username_style(role_list){
		return (typeof role_list !== "undefined" && role_list.length > 0) ? `color: #${role_list[0].color.toString(16)}` : "";
	}
}
