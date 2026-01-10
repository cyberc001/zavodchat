import axios from 'axios';
axios.defaults.withCredentials = true;

import Notifs from '$lib/notifs.svelte.js';

export default class Rest {
	static host = "https://127.0.0.1";

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
		return route;
	}
	static get_route_sur(server_id, user_id, role_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof user_id === "undefined")
			return route;
		route += "/users/" + user_id;
		if(typeof role_id === "undefined")
			return route;
		route += "/roles/" + role_id;
		return route;
	}
	static get_route_sr(server_id, role_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof role_id === "undefined")
			return route;
		route += "/roles/" + role_id;
		return route;
	}
	static get_route_sb(server_id, ban_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof ban_id === "undefined")
			return route;
		route += "/bans/" + ban_id;
		return route;
	}
	static get_route_si(server_id, invite_id)
	{
		let route = "";
		if(typeof server_id === "undefined")
			return route;
		route += "servers/" + server_id;
		if(typeof invite_id === "undefined")
			return route;
		route += "/invites/" + invite_id;
		return route;
	}

	static get_base_url()
	{
		return Rest.host + "/";
	}
	static params_to_query(params)
	{
		let query = "";
		for(let i = 0; i < params.length; i += 2)
			if(typeof params[i + 1] !== "undefined")
				query += (i == 0 ? "?" : "&") + params[i] + "=" + params[i + 1];
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

	static get(route, _then, _catch, ...params)
	{
		let headers = Rest.get_headers(params);
		axios.get(Rest.get_base_url() + route + Rest.params_to_query(params), headers)
			.then(_then)
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				_catch(err.response);
			});
	}
	static post(msg, route, content, _then, _catch, ...params)
	{
		let reqi = Notifs.add_notif(msg, Notifs.Types.Ongoing);
		let headers = Rest.get_headers(params);
		axios.post(Rest.get_base_url() + route + Rest.params_to_query(params),
				content, headers)
			.then((data) => {
				Notifs.remove_notif(reqi);
				_then(data);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				Notifs.remove_notif(reqi, Rest.err_to_str(err.response), msg);
				_catch(err.response);
			});
	}
	static put(msg, route, content, _then, _catch, ...params)
	{
		let reqi = Notifs.add_notif(msg, Notifs.Types.Ongoing);
		let headers = Rest.get_headers(params);
		axios.put(Rest.get_base_url() + route + Rest.params_to_query(params),
				content, headers)
			.then((data) => {
				Notifs.remove_notif(reqi);
				_then(data);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				Notifs.remove_notif(reqi, Rest.err_to_str(err.response), msg);
				_catch(err.response);
			}
		);
	}
	static delete(msg, route, _then, _catch, ...params)
	{
		let reqi = Notifs.add_notif(msg, Notifs.Types.Ongoing);
		let headers = Rest.get_headers(params);
		axios.delete(Rest.get_base_url() + route + Rest.params_to_query(params), headers)
			.then((data) => {
				Notifs.remove_notif(reqi);
				_then(data);
			})
			.catch((err) => {
				if(err.response === undefined)
					throw err;
				Notifs.remove_notif(reqi, Rest.err_to_str(err.response), msg);
				_catch(err.response);
			}
		);
	}
}
