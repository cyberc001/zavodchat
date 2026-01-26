export class IDObserver {
	data = $state({});
	loaded = $state(false);

	set_data(data){
		for(const key in data)
			this.data[key] = data[key];
		this.loaded = true;
	}
}

export class IDCache {
	cache = {};
	_default_state_constructor(){
		return new IDObserver();
	}

	// Keep references to state values so they dont get collected until interval passes
	_cache_refs = {};
	intv = setInterval(() => {
		for(const key in this._cache_refs)
			delete this._cache_refs[key];
	}, 60000);

	// ID: [int1, int2, ..., intN]
	state_refs_id(id){
		if(typeof id === "number")
			return id.toString();
		else if(typeof id === "object") // array
			return id.join();
	}

	get_state(_id, load_func){
		const id = this.state_refs_id(_id);
		let obj = this.cache[id];
		if(typeof obj === "undefined" || typeof obj.deref() === "undefined"){
			obj = this._default_state_constructor();
			this.cache[id] = new WeakRef(obj);
			if(load_func)
				load_func(this, id);
			return obj;
		}
		return obj.deref();
	}
	// Should be called from get_state(, load_func), therefore id is not parsed twice
	set_state(id, data){
		let obj = this.cache[id].deref();
		if(!obj){
			obj = this._default_state_constructor();
			this.cache[id] = new WeakRef(obj);	
		}
		obj.set_data(data);
		this._cache_refs[id] = obj;
	}
	has_state(_id){
		return typeof this.cache[this.state_refs_id(_id)]?.deref() !== "undefined";
	}
	remove_state(_id){
		const id = this.state_refs_id(_id);
		if(this.cache.hasOwnProperty(id))
			delete this.cache[id];
		delete this._cache_refs[id];
	}
}
