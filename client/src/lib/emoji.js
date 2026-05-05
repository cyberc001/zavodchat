import Util from '$lib/util.js';
import RestEmoji from '$lib/rest/emoji.js';

function __cmp_emojis(a, b){
	if(a.name < b.name)
		return -1;
	return a.name > b.name ? 1 : 0;
};

export default class Emoji {
	static __emoji_dict = {};
	static __emoji_array = [];
	static __emoji_groups = {};

	name; image;
	constructor(name, image)
	{
		this.name = name;
		this.image = image;
	}


	static convert_rest_emoji(e)
	{
		return {name: e.name, image: RestEmoji.get_image_path(e)};
	}


	static get(name, server_emojis)
	{
		if(server_emojis){
			const e = server_emojis.find((x) => x.name === name);
			if(e)
				return Emoji.convert_rest_emoji(e);
		}
		return Emoji.__emoji_dict[name];
	}

	static search(str_begin, server_emojis)
	{
		let res = Emoji.__search(str_begin);
		if(server_emojis){
			for(const e of server_emojis)
				if(e.name.startsWith(str_begin))
					res.push(Emoji.convert_rest_emoji(e));
			res.sort(__cmp_emojis);
		}
		return res;
	}
	static __search(str_begin)
	{
		if(str_begin.length === 0)
			return Emoji.__emoji_array;

		const idx = Util.bin_search(Emoji.__emoji_array, (a) => {
			if(a.name.startsWith(str_begin))
				return 0;
			return a.name < str_begin ? -1 : 1;
		});
		if(idx === -1)
			return [];

		let res = [];
		// Go backwards and forwards to find all other strings that begin with str_begin
		for(let i = idx - 1; i >= 0 &&
			Emoji.__emoji_array[i].name.startsWith(str_begin); --i)
			res.unshift(Emoji.__emoji_array[i]);
		res.push(Emoji.__emoji_array[idx]);
		for(let i = idx + 1; i < Emoji.__emoji_array.length &&
			Emoji.__emoji_array[i].name.startsWith(str_begin); ++i)
			res.push(Emoji.__emoji_array[i]);
		return res;
	}
};

// Load emojis
for(const path in import.meta.glob('/static/emoji/*/*.svg')){
	const dirc1 = path.lastIndexOf('/');
	const dirc2 = path.lastIndexOf('/', dirc1 - 1);

	const emoji = new Emoji(path.substring(dirc1 + 1).replace('.svg', ''),
				path.replace('/static', ''));
	Emoji.__emoji_dict[emoji.name] = emoji;
	Emoji.__emoji_array.push(emoji);

	const group = path.substring(dirc2 + 1, dirc1);
	if(typeof(Emoji.__emoji_groups[group]) === "undefined")
		Emoji.__emoji_groups[group] = [];
	Emoji.__emoji_groups[group].push(emoji);
}

// Order emojis
Emoji.__emoji_array.sort(__cmp_emojis);
for(const group of Object.values(Emoji.__emoji_groups))
	group.sort(__cmp_emojis);
