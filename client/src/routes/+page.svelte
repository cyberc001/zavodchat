<script>
	import {goto} from '$app/navigation';

	import Main from './main.svelte';
	import Server from '$lib/rest/server.js';

	let display_main = $state(false);
	
	// Check if authentificated
	Server.get_list_nocache(() => display_main = true,
				() => goto("/login")
	);
</script>

{#if display_main}
<Main />
{/if}

<style>
@import "main.css";
</style>