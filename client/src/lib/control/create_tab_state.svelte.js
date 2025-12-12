export default class CreateTabState {
	state = $state({});
	default_state = $state({});
	valid = $derived(true);

	constructor(_default_state){
		for(const key of Object.keys(_default_state))
			this.default_state[key] = this.state[key] = _default_state[key];
	}

	reset(keys){
		if(typeof keys !== "undefined"){
			for(const key of keys)
				this.state[key] = this.default_state[key];
			return;
		}
		for(const key of Object.keys(this.default_state))
			this.state[key] = this.default_state[key];
	}
}
