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
	static bin_search(arr, cmp_func){
		let l = 0, r = arr.length - 1;
		while(r - l > 1){
			const m = Math.floor((l + r) / 2);
			const c = cmp_func(arr[m]);
			if(c === 0)
				return m;
			else if(c > 0) 
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
	static bin_search_lower_bound(arr, cmp_func){
		if(arr.length === 0)
			return -1;

		let l = 0, r = arr.length - 1;
		while(r - l > 1){
			const m = Math.floor((l + r) / 2);
			const c = cmp_func(arr[m]);
			if(c === 0)
				return m;
			else if(c > 0) 
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

	static bin_search_lower_bound(arr, cmp_func){
		if(arr.length === 0)
			return 0;

		let l = 0, r = arr.length - 1;
		while(r - l > 1){
			const m = Math.floor((l + r) / 2);
			const c = cmp_func(arr[m]);
			if(c === 0)
				return m;
			else if(c > 0) 
				r = m; // left half remains
			else
				l = m; // right half remains
		}
		const c_l = cmp_func(arr[l]), c_r = cmp_func(arr[r]);
		if(c_l === 0)
			return l;
		if(c_r === 0)
			return r;
		if(c_l > 0)
			return l;
		if(c_r > 0)
			return r;
		return arr.length;
	}

	// Files
	static get_file_name(fpath){
		const i = fpath.lastIndexOf("/"), j = fpath.lastIndexOf("\\");
		if(i === -1 && j === -1)
			return fpath;
		return fpath.substring(Math.max(i, j) + 1);
	}
}
