import Util from '$lib/util.js';

function __cmp_emojis(a, b){
	if(a.name < b.name)
		return -1;
	return a.name > b.name ? 1 : 0;
};

export default class Emoji {
	static __emoji_dict = {};
	static __emoji_array = [];

	name; img_path;
	constructor(name, img_path)
	{
		this.name = name;
		this.img_path = img_path;
	}

	static get(name)
	{
		return Emoji.__emoji_dict[name];
	}
	static search(str_begin)
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
for(const path in import.meta.glob('/static/emoji/*.svg')){
	const emoji = new Emoji(path.replace('/static/emoji/', '').replace('.svg', ''),
				path.replace('/static', ''));
	Emoji.__emoji_dict[emoji.name] = emoji;
	Emoji.__emoji_array.push(emoji);
}
Emoji.__emoji_array.sort(__cmp_emojis);
