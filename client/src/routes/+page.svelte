<script>
	import Login from "../login.svelte";
	import Register from "../register.svelte";
	import Main from "../main.svelte";

	import Rest from '$lib/rest.js';
	import MainSocket from '$lib/socket/main.js';

	Rest.host = "https://127.0.0.1";
	MainSocket.host = "wss://127.0.0.1:444";

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
