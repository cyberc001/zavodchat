<script>
	import Server from '$lib/rest/server.js';

	const {server, selected = false,
		show_server, ctx_server} = $props();

	let self = $state();
</script>

<button class={"item hoverable sidebar_server" + (selected ? " selected" : "")}
	style="position: relative; anchor-name: --{"server_" + server.id}"
	onclick={() => show_server(server)}
	bind:this={self}
	oncontextmenu={(e) => {
		event.preventDefault();
		ctx_server(self, e, server);
	}}
>
	{#if server.avatar === undefined}
		<div style="padding:4px"><div class="sidebar_server_el">{server.name}</div></div>
	{:else}
		<img class="sidebar_server_el" alt={server.name} src={Server.get_avatar_path(server)}/>
	{/if}
	{#if typeof(server.notifications) !== "undefined"}
		<div class="notif_circle">{server.notifications}</div>
	{/if}
</button>

<style>
@import "server_element.css";
</style>
