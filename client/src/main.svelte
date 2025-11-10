<script>
	import Rest from '$lib/rest.js';
	import Server from '$lib/rest/server.js';

	let { setPage } = $props();

	const setError = (err) => {
		window.alert(err.status + " " + err.data);
	};

	// Client-side copy of data
	let data_servers = $state({});

	// Events
	let showServer = (id) => {
		window.alert(id);
	};

	// Initialization
	Server.get_list((list) => {
					let new_data_servers = {};
					for(let srv of list)
						new_data_servers[srv.id] = srv;
					data_servers = new_data_servers;
				}
				, setError);
</script>

<style>
	@import "main.css";
</style>

<div class="sidebar_servers">
	{#each Object.values(data_servers) as srv}
		{#if srv.avatar === undefined}
			<div class="icon_sidebar_server" onclick={() => showServer(srv.id)}><div class="icon_sidebar_server_text">{srv.name}</div></div>
		{:else}
			<img class="icon_sidebar_server" onclick={() => showServer(srv.id)} src={Server.get_avatar_path(srv)}/>
		{/if}
	{/each}
</div>
