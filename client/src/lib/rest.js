import axios from "axios";
axios.defaults.withCredentials = true;

export default class Rest {
	static host = "";

	static err_to_str(res)
	{
		let str = "";
		if(res.status < 400 || res.status > 499)
			str += res.status + " ";
	   	str += res.data;
		return str;
	}

	static get_base_url()
	{
		return Rest.host + "/";
	}
	static params_to_query(params)
	{
		let query = "";
		for(let i = 0; i < params.length; i += 2)
			query += (i == 0 ? "?" : "&") + params[i] + "=" + params[i+1];
		return query;
	}

	static get(route, _then, _catch, ...params)
	{
		axios.get(Rest.get_base_url() + route + Rest.params_to_query(params))
			.then(_then)
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			}
		);
	}
	static put(route, _then, _catch, ...params)
	{
		axios.put(Rest.get_base_url() + route + Rest.params_to_query(params))
			.then(_then)
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			}
		);
	}
}
