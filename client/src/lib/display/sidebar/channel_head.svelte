<script>
	import {asset} from '$app/paths';

	import SearchBar from '$lib/control/search_bar.svelte';

	import User from '$lib/rest/user.svelte.js';
	import Channel from '$lib/rest/channel.js';
	import DM from '$lib/rest/dm.js';

	const {channel, server_id,
		onsearch, show_channel} = $props();

	let other_user = $state();
	$effect(() => {
		if(channel.loaded && typeof(channel.data.other_user_id) !== "undefined")
			other_user = User.get(channel.data.other_user_id);
		else
			other_user = undefined;
	});

	let is_loaded = $derived(channel.loaded && (!other_user || other_user.loaded));
	let name = $derived(other_user ? other_user.data.name : channel.data.name);

	let search_bar = $state();
	export function reset(){
		search_bar.reset();
	}
</script>

<div class="channel_head">
{#if is_loaded}
	{name}
{:else}
	&nbsp;
{/if}

<div style="margin-left: auto; display: flex">
	{#if other_user}
		<button class="transparent_button hoverable"
		style="margin-right: 4px"
		onclick={() => {
			let dm = DM.open(other_user.data.id);
			dm.notify_on_load(() => {
				const ch = Channel.get(dm.data[1]);
				ch.notify_on_load(() => {
					show_channel(ch.data);
				});
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
