<script>
	import {untrack} from 'svelte';

	import {asset} from '$app/paths';
	import SidebarChannelElement from '$lib/display/sidebar/channel_element.svelte';
	import SidebarChannelAction from '$lib/display/sidebar/channel_action.svelte';
	import DMElement from '$lib/display/sidebar/dm.svelte';
	import ServerHead from '$lib/display/sidebar/server_head.svelte';

	import MediaDisplay from '$lib/display/media.svelte';
	import OrderedList from '$lib/control/ordered_list.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	import Channel from '$lib/rest/channel.js';
	import Role from '$lib/rest/role.js';
	import User from '$lib/rest/user.svelte.js';
	import DM from '$lib/rest/dm.js';
	import Notifications from '$lib/rest/notifications.js';

	const {server, selected_channel,
		socket_vc,
		show_channel, ctx_channel, ctx_vc_user,
		create_channel} = $props();

	let channels = $state();
	let server_roles = $state();
	let user_self = $state();
	$effect(() => {
		if(server?.loaded){
			channels = Channel.get_list(server.data.id);
			user_self = undefined;
			server_roles = Role.get_list(server.data.id);
			User.get_self_server(server, (user) => user_self = user);
		} else {
			channels = typeof(server) === "undefined" ? undefined : {loaded: false};
			server_roles = undefined;
		}
	});

	let can_manage_channels = $derived(user_self?.loaded && server_roles?.loaded &&
						Role.check_perms(user_self.data, server.data, server_roles.data, 1, 2));

	let vc_video_elem = $state();
	$effect(() => {
		if(socket_vc?.get_watched_video() && vc_video_elem){
			vc_video_elem.srcObject = new MediaStream([socket_vc.get_watched_video()]);
			vc_video_elem.play();
		}
	});

	// Remove notifications on an actively read DM channel
	let channel = $state();
	$effect(() => {
		if(selected_channel > -1)
			channel = Channel.get(selected_channel);
		else
			channel = undefined;
	});
	$effect(() => {
		if(channel?.loaded && typeof(channel.data.notifications) !== "undefined")
			Notifications.remove_channel(channel.data.id, server?.data?.id);
	});
</script>


{#snippet render_channel(i, ch)}
<SidebarChannelElement channel={ch} server={server} socket_vc={socket_vc}
	selected={selected_channel === ch.id} last={i == channels.data.length - 1}
	show_channel={(ch) => show_channel(ch)}
	ctx_channel={ctx_channel} ctx_vc_user={ctx_vc_user}
/>
{/snippet}

<div style="display: flex; flex-direction: column">
	<div class="panel sidebar_channels">
		{#if server}
			<ServerHead server={server}/>
	
			{#if typeof(channels) === "undefined"}
			{:else if !channels.loaded}
				<div style="text-align: center; margin-top: 6px">
					<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 36px"/>
				</div>
			{:else}
				<OrderedList items={channels.data}
				check_select={() => can_manage_channels}
				on_drag={(dragged, dragged_idx, hovered, hovered_idx) => {
					Channel.change(dragged.id, {prev_channel_id: (typeof(hovered) === "undefined" ? -1 : hovered.id)},
							() => {});
					return false;
				}}
				render_item={render_channel}
				/>
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
	{#if channels?.loaded && can_manage_channels}
		<div class="panel sidebar_channels sidebar_channel_actions">
			<SidebarChannelAction
				icon={asset("icons/add.svg")} text="Add channel"
				action={create_channel}
			/>
		</div>
	{/if}
</div>

{#if socket_vc?.get_watched_video()}
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
