export default class {
	cache = $state({});
	_default_state_constructor(){
		return {};
	}

	// ID: [int1, int2, ..., intN]
	state_refs_id(id){
		if(typeof id === "number")
			return id.toString();
		else if(typeof id === "object") // array
			return id.join();
	}

	get_state(_id, load_func){
		let id = this.state_refs_id(_id);
		if(typeof this.cache[id] === "undefined"){
			this.cache[id] = this._default_state_constructor();
			if(load_func)
				load_func(this, id);
		}
		return this.cache[id];
	}
	// Should be called from get_state(, load_func), therefore id is not parsed twice
	set_state(id, data){
		for(const key in data)
			this.cache[id][key] = data[key];
	}
}
