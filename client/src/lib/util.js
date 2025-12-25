export default class Util {
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
};
