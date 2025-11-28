import IDCache from "$lib/cache/id.svelte.js";

export default class extends IDCache {
	_default_state_constructor(){
		return [];
	}
	// Should be called from get_state(, load_func), therefore id is not parsed twice
	set_state(id, data){
		console.log("splicing", this.cache, id);
		this.cache[id].splice(0, this.cache[id].length);
		for(const d of data)
			this.cache[id].push(d);
	}
}
