<script>
	import Login from "../login.svelte";
	import Register from "../register.svelte";

	let page = $state(0);
	let setPage = (p) => page = p;
</script>

{#if page == 0}
	<Login setPage={setPage}/>
{:else if page == 1}
	<Register setPage={setPage}/>
{/if}
