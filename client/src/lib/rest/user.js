let route = "users";
import Rest from "$lib/rest";

export default class {
	static get(id, _then, _catch){
		Rest.get(route + "/" + id, (res) => _then(res.data), _catch);
	}

	static get_avatar_path(usr){
		return Rest.get_base_url() + "files/avatar/user/" + usr.avatar;
	}
}
