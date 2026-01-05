import Util from "$lib/util.js";

export default class SettingsTabState {
	state = $state({});
	default_state = $state({});

	copy_obj(obj){
		if(obj instanceof File)
			return obj;
		if(Array.isArray(obj)){
			let new_arr = [];
			for(const e of obj)
				new_arr.push(this.copy_obj(e));
			return new_arr;
		}
		if(typeof obj === "object" && obj){ // null is also an object
			let new_obj = {};
			for(const key in obj)
				new_obj[key] = obj[key];
			return new_obj;
		}
		return obj;
	}

	constructor(_default_state){
		for(const key in _default_state){
			this.default_state[key] = this.copy_obj(_default_state[key]);
			this.state[key] = this.copy_obj(_default_state[key]);
		}
	}

	changes_override = $state(SettingsTabState.ChangesState.Inherit);
	changes = $derived.by(() => {
		if(this.changes_override !== SettingsTabState.ChangesState.Inherit)
			return this.changes_override;
		for(const key in this.state)
			if(this.is_changed(key))
				return SettingsTabState.ChangesState.HasChanges;
		return SettingsTabState.ChangesState.NoChanges;
	});

	is_changed(key){
		return !Util.deep_equals(this.state[key], this.default_state[key]);
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
			this.state[key] = this.copy_obj(this.default_state[key]);
	}


	set_all_states(key, value){
		this.default_state[key] = this.copy_obj(value);
		this.state[key] = this.copy_obj(value);
	}
	set_default_state(key, value){
		if(Util.deep_equals(this.default_state[key], value))
			return;

		if(!this.is_changed(key))
			this.state[key] = this.copy_obj(value);
		this.default_state[key] = this.copy_obj(value);
	}


	static ChangesState = {
		Inherit: 0,
		Invalid: 1,
		HasChanges: 2,
		NoChanges: 3
	}
}
