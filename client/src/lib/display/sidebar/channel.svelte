<script>
	import SidebarChannelElement from '$lib/display/sidebar/channel_element.svelte';
	import SidebarChannelAction from '$lib/display/sidebar/channel_action.svelte';

	const {server, channels, selected_channel,
		socket_vc,
		show_channel, ctx_channel, ctx_vc_user,
		create_channel} = $props();
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
					<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
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
			<SidebarChannelAction last={true}
				icon="/src/lib/assets/icons/add.svg" text="Add channel"
				action={create_channel}
			/>
		</div>
	{/if}
</div>

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
	margin: 8px 0 8px 8px;
	font-size: 24px;
}
</style>
