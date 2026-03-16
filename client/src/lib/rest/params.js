import Rest from '$lib/rest.js';

export default class Params {
	static data = {};

	// Should be called once, when client is loaded for the first time
	static load(_catch){
		Rest.get("params", (res) => {
			for(const key of Object.keys(res.data))
				Params.data[key] = res.data[key];
		}, _catch);
	}
}
