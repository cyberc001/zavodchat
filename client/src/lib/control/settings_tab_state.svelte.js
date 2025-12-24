export default class SettingsTabState {
	state = $state({});
	default_state = $state({});

	copy_obj(obj){
		if(Array.isArray(obj))
			return obj.slice();
		return obj;
	}

	constructor(_default_state){
		for(const key of Object.keys(_default_state)){
			this.default_state[key] = this.copy_obj(_default_state[key]);
			this.state[key] = this.copy_obj(_default_state[key]);
		}
	}

	changes_override = $state(SettingsTabState.ChangesState.Inherit);
	changes = $derived.by(() => {
		if(this.changes_override !== SettingsTabState.ChangesState.Inherit)
			return this.changes_override;
		for(const key of Object.keys(this.state))
			if(this.is_changed(key))
				return SettingsTabState.ChangesState.HasChanges;
		return SettingsTabState.ChangesState.NoChanges;
	});

	is_changed(key){
		const a = this.state[key];
		const b = this.default_state[key];

		if(Array.isArray(a) && Array.isArray(b))
			return a.length !== b.length || a.some((v, i) => v !== b[i]);
		return a !== b;
	}
	get_dict_of_changes(allowed){
		let dict = {};
		for(const key in (typeof allowed === "undefined" ? this.state : allowed))
			if(this.is_changed(key))
				dict[key] = this.state[key];
		return dict;
	}

	apply_changes(){
		for(const key of Object.keys(this.state))
			this.default_state[key] = this.copy_obj(this.state[key]);
	}
	discard_changes(keys){
		if(typeof keys !== "undefined"){
			for(const key of keys)
				this.state[key] = this.copy_obj(this.default_state[key]);
			return;
		}

		for(const key of Object.keys(this.default_state))
			this.state[key] = this.copyObj(this.default_state[key]);
	}


	set_all_states(key, value){
		this.default_state[key] = this.copy_obj(value);
		this.state[key] = this.copy_obj(value);
	}


	static ChangesState = {
		Inherit: 0,
		Invalid: 1,
		HasChanges: 2,
		NoChanges: 3
	}
}
