<script>
	import Login from '../login.svelte';
	import Register from '../register.svelte';
	import Main from '../main.svelte';

	import Rest from '$lib/rest.svelte.js';
	import MainSocket from '$lib/socket/main.js';

	let page = $state(0);
	let setPage = (p) => page = p;
</script>

{#if page == 0}
	<Login setPage={setPage}/>
{:else if page == 1}
	<Register setPage={setPage}/>
{:else if page == 2}
	<Main setPage={setPage}/>
{/if}
