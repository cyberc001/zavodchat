import adapter from '@sveltejs/adapter-static';
import { importAssets } from 'svelte-preprocess-import-assets';

/** @type {import('@sveltejs/kit').Config} */
const config = {
	preprocess: [importAssets()],
	kit: {
		adapter: adapter({
			fallback: '200.html' // may differ from host to host
		})
	}
};

export default config;
