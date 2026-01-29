import Rest from '$lib/rest.js';

export default class Preferences {
	data = $state({
		loaded: false,

		noise_supression: "rnnoise"
	});
	static _inst = new Preferences();
	static get data(){
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

Rest.get("preferences",
	(res) => {
			for(const key of Object.keys(res.data))
				Preferences.data[key] = res.data[key];
			Preferences.data.loaded = true;
	},
	() => {});
