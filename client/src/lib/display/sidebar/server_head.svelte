<script>
	import {asset} from '$app/paths';

	const {server} = $props();
</script>

<div class="server_head">
{#if server?.loaded}
	{server.data.name}
{:else if typeof(server) !== "undefine"}
	<div style="text-align: center;">
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 36px"/>
	</div>
{/if}
</div>

<style>
.server_head {
	height: 1lh;
	padding: 8px;

	border-width: 2px;
	border-style: none none solid none;
	border-color: var(--clr_border);

	font-size: 24px;
}
</style>
