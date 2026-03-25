import {IDCache, IDObserver} from '$lib/cache/id.svelte.js';
import User from '$lib/rest/user.svelte.js';

export class ListObserver extends IDObserver {
	data = $state([]);

	set_data(data){
		this.data.splice(0, this.data.length);
		for(const d of data)
			this.data.push(d);
		this.loaded = true;
	}
}

export class ListCache extends IDCache {
	_default_state_constructor(){
		return new ListObserver();
	}

	add_to_list(_id, data){
		const id = this.state_refs_id(_id);
		if(this.has_state(id))
			this.get_state(id).data.push(data);
	}
	remove_from_list(_id, data_id){
		const id = this.state_refs_id(_id);
		if(this.has_state(id)){
			const data = this.get_state(id).data;
			let i = data.findIndex((x) => x.id === data_id || x.data?.id === data_id);
			if(i !== -1)
				data.splice(i, 1);
		}
	}

	add_user_id_to_list(_id, user_id){
		const user = User.get(user_id);
		user.id = user_id;
		this.add_to_list(_id, user);
	}
}
