<script>
	import {asset} from '$app/paths';

	import SearchBar from '$lib/control/search_bar.svelte';

	import Channel from '$lib/rest/channel.js';

	const {channel, server_id,
		onsearch, show_channel} = $props();

	let search_bar = $state();
	export function reset(){
		search_bar.reset();
	}
</script>

<div class="channel_head">
{#if channel.loaded}
	{channel.data.name}
{:else}
	&nbsp;
{/if}

<div style="margin-left: auto; display: flex">
	{#if typeof(channel.data.linked_vc_id) !== "undefined"}
		<button class="transparent_button hoverable"
		style="margin-right: 4px"
		onclick={() => {
			const ch = Channel.get(channel.data.linked_vc_id);
			ch.notify_on_load(() => {
				ch.data.name = channel.data.name;
				show_channel(ch.data);
			});
		}}>
			<img src={asset("icons/call.svg")} alt="call friend" class="filter_icon_main" style="width: 32px"/>
		</button>
	{/if}
	<SearchBar server_id={server_id} bind:this={search_bar}
		elements={[
			{type: server_id > -1 ? "server_user" : "user", label: "Author", param: "author_id"},
			{type: "date", label: "Date from", param: "date_from"},
			{type: "date", label: "Date until", param: "date_until"}
		]}
		onsearch={onsearch}
		--margin-bottom="0px"
	/>
</div>
</div>

<style>
.channel_head {
	display: flex;
	height: 1lh;

	padding: 8px;

	border-width: 2px;
	border-style: none none solid none;
	border-color: var(--clr_border);

	font-size: 24px;
}
</style>
