export default class SettingsTabState {
	state = $state({});
	default_state = $state({});

	constructor(_default_state){
		for(const key of Object.keys(_default_state))
			this.default_state[key] = this.state[key] = _default_state[key];
	}

	changes_override = $state(SettingsTabState.ChangesState.Inherit);
	changes = $derived.by(() => {
		if(this.changes_override !== SettingsTabState.ChangesState.Inherit)
			return this.changes_override;
		for(const key of Object.keys(this.state))
			if(this.state[key] !== this.default_state[key])
				return SettingsTabState.ChangesState.HasChanges;
		return SettingsTabState.ChangesState.NoChanges;
	});

	is_changed(key){
		return this.state[key] !== this.default_state[key];
	}

	apply_changes(){
		for(const key of Object.keys(this.state))
			this.default_state[key] = this.state[key];
	}
	discard_changes(keys){
		if(typeof keys !== "undefined"){
			for(const key of keys)
				this.state[key] = this.default_state[key];
			return;
		}

		for(const key of Object.keys(this.default_state))
			this.state[key] = this.default_state[key];
	}


	set_all_states(key, value){
		this.state[key] = this.default_state[key] = value;
	}


	static ChangesState = {
		Inherit: 0,
		Invalid: 1,
		HasChanges: 2,
		NoChanges: 3
	}
}
