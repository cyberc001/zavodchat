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

	static get_route_scm(server_id, channel_id, message_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof channel_id === "undefined")
			return route;
		route += "/channels/" + channel_id;
		if(typeof message_id === "undefined")
			return route;
		route += "/messages/" + message_id;
		return route
	}
	static get_route_su(server_id, user_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof user_id === "undefined")
			return route;
		route += "/users/" + user_id;
		return route
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
	static get_headers(params)
	{
		if(params.length > 0 && typeof params[0] === "object"){
			let headers = params[0];
			params.splice(0, 1);
			return {"headers": headers};
		}
		return {};
	}

	static outgoing_requests = {};
	static cancel_request(request_name)
	{
		if(typeof Rest.outgoing_requests[request_name] !== "undefined"){
			Rest.outgoing_requests[request_name].abort();
			delete Rest.outgoing_requests[request_name];
		}
	}

	static get(request_name, route, _then, _catch, ...params)
	{
		let headers = Rest.get_headers(params);
		let abort_controller = new AbortController();
		headers.signal = abort_controller.signal;
		axios.get(Rest.get_base_url() + route + Rest.params_to_query(params), headers)
			.then(async (res) => {
				delete Rest.outgoing_requests[request_name];
				_then(res);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			});
		Rest.outgoing_requests[request_name] = abort_controller;
	}
	static post(request_name, route, content, _then, _catch, ...params)
	{
		let headers = Rest.get_headers(params);
		let abort_controller = new AbortController();
		headers.signal = abort_controller.signal;
		axios.post(Rest.get_base_url() + route + Rest.params_to_query(params),
				content, headers)
			.then(async (res) => {
				delete Rest.outgoing_requests[request_name];
				_then(res);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			});
		Rest.outgoing_requests[request_name] = abort_controller;
	}
	static put(request_name, route, content, _then, _catch, ...params)
	{
		let headers = Rest.get_headers(params);
		let abort_controller = new AbortController();
		headers.signal = abort_controller.signal;
		axios.put(Rest.get_base_url() + route + Rest.params_to_query(params),
				content, headers)
			.then(async (res) => {
				delete Rest.outgoing_requests[request_name];
				_then(res);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			}
		);
		Rest.outgoing_requests[request_name] = abort_controller;
	}
	static delete(request_name, route, _then, _catch, ...params)
	{
		let headers = Rest.get_headers(params);
		let abort_controller = new AbortController();
		headers.signal = abort_controller.signal;
		axios.delete(Rest.get_base_url() + route + Rest.params_to_query(params), headers)
			.then(async (res) => {
				delete Rest.outgoing_requests[request_name];
				_then(res);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			}
		);
		Rest.outgoing_requests[request_name] = abort_controller;
	}
}
