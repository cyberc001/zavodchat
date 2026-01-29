<script>
	import {asset} from '$app/paths';
	import SidebarServerElement from '$lib/display/sidebar/server_element.svelte';
	import SidebarServerAction from '$lib/display/sidebar/server_action.svelte';

	const {servers, selected_server,
		show_server, ctx_server,
		create_server} = $props();
</script>

<div style="display: flex; flex-direction: column">
	<div class="panel sidebar_servers">
		{#if !servers.loaded}
			<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 48px"/>
		{:else}
			{#each servers.data as srv, i}
				<SidebarServerElement server={srv} selected={selected_server === srv.id}
					show_server={(id) => show_server(id, i)}
					ctx_server={ctx_server}
				/>
			{/each}
		{/if}
	</div>
	<div class="panel sidebar_servers sidebar_server_actions">
		<SidebarServerAction icon={asset("icons/add.svg")} action={create_server}/>
	</div>
</div>

<style>
.sidebar_servers {
	display: flex;
	flex-direction: column;
	align-items: center;

	border-style: none solid none none;
	box-sizing: border-box;

	width: 64px;
	height: 100%;
	padding: 6px;
}
.sidebar_server_actions {
	height: 64px;
}
</style>
