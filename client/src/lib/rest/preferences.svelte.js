import Rest from '$lib/rest.js';

export default class Preferences {
	data = $state({
		loaded: false,
		loading: false,

		noise_supression: "rnnoise"
	});
	static _inst = new Preferences();
	static get data(){
		if(!Preferences._inst.loaded && !Preferences._inst.loading){
			Preferences._inst.loading = true;
			Rest.get("preferences",
				(res) => {
						for(const key of Object.keys(res.data))
							Preferences.data[key] = res.data[key];
						Preferences.data.loaded = true;
						Preferences.data.loading = false;
				},
			() => {});
		}
		return Preferences._inst.data;
	}

	static change(changes, _then, _catch){
		Rest.put("Changing preferences", "preferences",
				changes,
				(res) => {
					for(const key of Object.keys(changes))
						Preferences.data[key] = changes[key];
					_then();
				}, _catch);
	}
}