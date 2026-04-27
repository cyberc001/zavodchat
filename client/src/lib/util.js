export default class Util {
	// Objects
	static form_data_from_object(obj, allowed_props){
		if(typeof allowed_props === "undefined")
			allowed_props = Object.keys(obj);
		let fd = new FormData();
		for(const key of allowed_props)
			if(obj.hasOwnProperty(key) && obj[key] !== null && typeof obj[key] !== "undefined")
				fd.append(key, obj[key]);
		return fd;
	}
	static object_from_object(obj, allowed_props){
		if(typeof allowed_props === "undefined")
			allowed_props = Object.keys(obj);
		let data = {};
		for(const key of allowed_props)
			if(obj.hasOwnProperty(key) && obj[key] !== null && typeof obj[key] !== "undefined")
				data[key] = obj[key];
		return data;
	}

	static deep_equals(a, b){
		if(Array.isArray(a) && Array.isArray(b))
			return a.length === b.length && a.every((v, i) => Util.deep_equals(v, b[i]));
		if(typeof a === "object" && typeof b === "object" && a && b){ // null is also an object
			const k1 = Object.keys(a), k2 = Object.keys(b);
			if(k1.length !== k2.length)
				return false;
			for(let i = 0; i < k1.length; ++i){
				const key = k1[i];
				if(!Util.deep_equals(a[key], b[key]))
					return false;
			}
			return true;
		}
		return a === b;
	}

	// Time
	static TimeUnits = {
		Seconds: 0,
		Minutes: 1,
		Hours: 2,
		Days: 3
	}
	static time_unit_mul(unit){
		switch(unit){
			case Util.TimeUnits.Seconds: return 1000;
			case Util.TimeUnits.Minutes: return 60000;
			case Util.TimeUnits.Hours: return 3600000;
			case Util.TimeUnits.Days: return 86400000;
		}
	}
	static date_add(date, interval, units){
		return new Date(date + interval * Util.time_unit_mul(units));
	}

	// Formatting
	static padded(num, ln){
		return num.toString().padStart(ln, "0");
	}
	static padded_hex(num, ln){
		if(!ln)
			ln = 2;
		return Math.round(num).toString(16).padStart(ln, "0");
	}
	
	// Algorithms
	static bin_search(arr, cmp_func, reverse){
		let l = 0, r = arr.length - 1;
		while(r - l > 1){
			const m = Math.floor((l + r) / 2);
			const c = cmp_func(arr[m]);
			console.log("l", l, "r", r, "c", c, arr[m]);
			if(c === 0)
				return m;
			else if(c * (reverse ? -1 : 1) > 0)
				r = m; // left half remains
			else
				l = m; // right half remains
		}
		if(cmp_func(arr[l]) === 0)
			return l;
		if(r > l && cmp_func(arr[r]) === 0)
			return r;
		return -1;
	}

	// Files
	static get_file_name(fpath){
		const i = fpath.lastIndexOf("/"), j = fpath.lastIndexOf("\\");
		if(i === -1 && j === -1)
			return fpath;
		return fpath.substring(Math.max(i, j) + 1);
	}

	// Web
	static __pending_fetches = {};
	static __pending_fetch_last_id = 0;
	static group_fetch(name, resource, options, _then, _catch){
		// TODO maybe delete empty sets
		if(typeof Util.__pending_fetches[name] === "undefined")
			Util.__pending_fetches[name] = new Set();
		const id = ++Util.__pending_fetch_last_id;
		Util.__pending_fetches[name].add(id);
		fetch(resource, options).then((res) => {
			if(typeof Util.__pending_fetches[name] !== "undefined" && Util.__pending_fetches[name].has(id)){
				Util.__pending_fetches[name].delete(id);
				_then(res);
			}
		}).catch(() => {
			if(typeof Util.__pending_fetches[name] !== "undefined")
			Util.__pending_fetches[name].delete(id);
			_catch(res);
		});
	}
	static cancel_fetch_group(name)
	{
		delete Util.__pending_fetches[name];
	}

	static get_page_meta(html){
		const parser = new DOMParser();
		const doc = parser.parseFromString(html, "text/html");
		if(!doc.title)
			return;
		const all_meta = doc.getElementsByTagName("meta");
		for(const meta of all_meta)
			if(meta.name === "description")
				return {title: doc.title, desc: meta.content};
		return doc.title;
	}

	// Other stuff
	static inc_or_set(obj, key, val)
	{
		if(!val)
			val = 1;

		if(typeof(obj[key]) === "undefined")
			obj[key] = val;
		else
			obj[key] += val;
	}
	static set_if_absent(obj, key, val)
	{
		if(typeof(obj[key]) === "undefined")
			obj[key] = val;
	}

	static is_ws(c)
	{
		return c === ' '
		    || c === '\n' || c === '\t' || c === '\r' || c === '\f' || c === '\v'
		    || c === '\u00a0' || c === '\u1680' || c === '\u2000' || c === '\u200a' || c === '\u2028' || c === '\u2029' || c === '\u202f' || c === '\u205f' || c === '\u3000' || c === '\ufeff';
	}
}
