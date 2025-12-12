export default class {
	static form_data_from_object(obj, allowed_props)
	{
		if(typeof allowed_props === "undefined")
			allowed_props = Object.keys(obj);
		let fd = new FormData();
		for(const key of allowed_props)
			if(obj.hasOwnProperty(key) && obj[key] !== null && typeof obj[key] !== "undefined")
				fd.append(key, obj[key]);
		return fd;
	}
	static object_from_object(obj, allowed_props)
	{
		if(typeof allowed_props === "undefined")
			allowed_props = Object.keys(obj);
		let data = {};
		for(const key of allowed_props)
			if(obj.hasOwnProperty(key) && obj[key] !== null && typeof obj[key] !== "undefined")
				data[key] = obj[key];
		return data;
	}
};
