import {IDCache, IDObserver} from "$lib/cache/id.svelte.js";

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
}
