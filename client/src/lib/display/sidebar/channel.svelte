<script>
	import {asset} from '$app/paths';
	import SidebarChannelElement from '$lib/display/sidebar/channel_element.svelte';
	import SidebarChannelAction from '$lib/display/sidebar/channel_action.svelte';
	import MediaDisplay from '$lib/display/media.svelte';

	const {server, channels, selected_channel,
		socket_vc,
		show_channel, ctx_channel, ctx_vc_user,
		create_channel} = $props();

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
			<div class="sidebar_channel_name">
				{#if server.data?.name}
					{server.data?.name}
				{/if}
			</div>
	
			{#if !channels.loaded}
				<div style="text-align: center; margin-top: 6px">
					<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 48px"/>
				</div>
			{:else}
				{#each channels.data as ch, i}
					<SidebarChannelElement channel={ch} socket_vc={socket_vc}
						selected={selected_channel === ch.id} last={i == channels.data.length - 1}
						show_channel={(id) => show_channel(id, i)}
						ctx_channel={ctx_channel} ctx_vc_user={ctx_vc_user}
					/>
				{/each}
			{/if}

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
.sidebar_channel_name {
	padding: 8px 0 8px 8px;

	border-width: 2px;
	border-style: none none solid none;
	border-color: var(--clr_border);

	font-size: 24px;
}
</style>
