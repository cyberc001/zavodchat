<script>
	import {asset} from '$app/paths';
	import SidebarChannelElement from '$lib/display/sidebar/channel_element.svelte';
	import SidebarChannelAction from '$lib/display/sidebar/channel_action.svelte';
	import DMElement from '$lib/display/sidebar/dm.svelte';
	import ServerHead from '$lib/display/sidebar/server_head.svelte';

	import MediaDisplay from '$lib/display/media.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	import Channel from '$lib/rest/channel.js';
	import DM from '$lib/rest/dm.js';

	const {server, selected_channel,
		socket_vc,
		show_channel, ctx_channel, ctx_vc_user,
		create_channel} = $props();

	let channels = $derived(server?.loaded ? Channel.get_list(server.data.id) : undefined);

	let vc_video_elem = $state();
	$effect(() => {
		if(socket_vc?.watched_video && vc_video_elem){
			vc_video_elem.srcObject = new MediaStream([socket_vc.watched_video]);
			vc_video_elem.play();
		}
	});
</script>

<div style="display: flex; flex-direction: column">
	<div class="panel sidebar_channels">
		{#if channels}
			<ServerHead server={server} />
	
			{#if !channels.loaded}
				<div style="text-align: center; margin-top: 6px">
					<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 48px"/>
				</div>
			{:else}
				{#each channels.data as ch, i}
					<SidebarChannelElement channel={ch} socket_vc={socket_vc}
						selected={selected_channel === ch.id} last={i == channels.data.length - 1}
						show_channel={(ch) => show_channel(ch)}
						ctx_channel={ctx_channel} ctx_vc_user={ctx_vc_user}
					/>
				{/each}
			{/if}
		{:else if !server}
			{#snippet render_dm_channel(i, item)}
				<DMElement channel={item}
				show_channel={(ch) => show_channel(ch)}
				/>
			{/snippet}
			<PaginatedList
			render_item={render_dm_channel}
			load_items={(start_id, range, asc) => DM.get_channels(start_id, range, asc)}
			/>
		{/if}
	</div>
	{#if channels?.loaded}
		<div class="panel sidebar_channels sidebar_channel_actions">
			<SidebarChannelAction
				icon={asset("icons/add.svg")} text="Add channel"
				action={create_channel}
			/>
		</div>
	{/if}
</div>

{#if socket_vc && socket_vc.watched_video}
	<MediaDisplay close_media={() => socket_vc.unwatch_video()}>
		<video class="fullscreen_media" bind:this={vc_video_elem}/>
	</MediaDisplay>
{/if}


<style>
.sidebar_channels {
	width: 256px;
	height: 100%;
	border-style: none solid none none;
}
.sidebar_channel_actions {
	height: 32px;
}
</style>
