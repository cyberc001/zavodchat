<script>
	import {asset} from '$app/paths';
	import Util from '$lib/util.js';

	import Channel from '$lib/rest/channel.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	import UserDisplay from '$lib/display/user.svelte';

	import {VideoState} from '$lib/socket/vc_utils.svelte.js';

	const {selected, last,
		channel, server, socket_vc,
		show_channel, ctx_channel, ctx_vc_user} = $props();

	let self = $state();

	let user_self = $state();
	let server_roles = $state();
	$effect(() => {
		if(server?.loaded){
			server_roles = Role.get_list(server.data.id);
			user_self = undefined;
			User.get_self_server(server, (user) => user_self = user);
		}
		else {
			server_roles = undefined;
			user_self = undefined;
		}
	});

	let can_join_vc = $derived(user_self?.loaded && server_roles?.loaded &&
					Role.check_perms(user_self.data, server.data, server_roles.data, 1, 7));
	let private_suffix = $derived(channel.wl_users.length > 0 || channel.wl_roles.length > 0 ? "_private" : "");
	let is_text_secondary = $derived(channel.type === Channel.Type.Text ?
						typeof(channel.notifications) === "undefined" :
						!can_join_vc);

	const get_talk_border = (vc_state) => "border-style: solid; border-width: 2px; border-color: " +
						(socket_vc && socket_vc.get_audio(vc_state.user.data.id) ?
						"#00FF00" + Util.padded_hex(Math.min(socket_vc.get_audio(vc_state.user.data.id).amplitude / 10, 1) * 255) :
						"#00000000");
</script>

<div>
	<button class={"item hoverable transparent_button sidebar_channel_el" + (selected ? " selected" : "")}
		style={"border-style: none none solid none; anchor-name: --channel_" + channel.id
			+ "; padding: 4px 0px 4px 4px; border-color: var(--clr_border)"}
		onclick={() => show_channel(channel)}
		disabled={channel.type === Channel.Type.Voice && !can_join_vc}
		bind:this={self}
		oncontextmenu={(e) => {
			event.preventDefault();
			ctx_channel(self, e, channel);
		}}
	>
		{#if channel.type === Channel.Type.Voice}
			<img src={asset(`icons/channel_vc${private_suffix}.svg`)} alt="voice_channel" class="filter_icon_main sidebar_channel_el_icon"/>
		{:else}
			<img src={asset(`icons/channel_text${private_suffix}.svg`)}  alt="text_channel" class="filter_icon_main sidebar_channel_el_icon"/>
		{/if}
		<div class="content"
		style={`color: var(--clr_text${is_text_secondary ? "_secondary" : ""})`}>
		{channel.name}
		{#if typeof(channel.notifications) !== "undefined" && channel.notifications > 0}
			<div class="notif_circle channel_notif_circle">{channel.notifications}</div>
		{/if}
		</div>
	</button>
	{#if channel.type === Channel.Type.Voice && typeof channel.vc_users === "object"}
		{#each Object.values(channel.vc_users) as vc_state}
			<div class="vc_user_panel">
				<UserDisplay user={vc_state.user} server={server}
					id={"vc_user_display_" + vc_state.user.data.id}
					display_status={false}
					show_ctx_menu={(anchor, e) => ctx_vc_user(anchor, e, channel.id, vc_state)}

					avatar_style={get_talk_border(vc_state)}
					--margin-left="0"
					--width="calc(100% - 80px)"
				/>

				<div style="margin-left: auto; margin-right: 4px; display: flex; align-items: center">
					{#if vc_state.video > VideoState.Disabled}
						{#if socket_vc && socket_vc.get_video(vc_state.user.data.id)}
						<button class="hoverable transparent_button" style="display: flex"
							onclick={() => {
								socket_vc.watch_video(vc_state.user.data.id);
							}}
						>
							<img src={asset("icons/screen_share.svg")} alt="watch screen share" class="filter_icon_highlight vc_state_icon"/>
						</button>
						{:else}
							<img src={asset("icons/screen_share.svg")} alt="sharing screen" class="filter_icon_main vc_state_icon"/>
						{/if}
					{/if}

					{#if vc_state.mute}
						<img src={asset("icons/muted.svg")} alt="muted" class="filter_icon_main vc_state_icon"/>
					{/if}
					{#if vc_state.deaf}
						<img src={asset("icons/deaf.svg")} alt="deaf" class="filter_icon_main vc_state_icon"/>
					{/if}
				</div>
			</div>
		{/each}
	{/if}
</div>

<style>
@import "channel_element.css";

.content {
	width: 100%;
	display: flex;
}
.channel_notif_circle {
	position: relative;
	right: unset;
	bottom: unset;
	margin-left: auto;
	margin-right: 5px;

	font-size: 18px;
}

.vc_user_panel {
	display: flex;
	align-items: center;
	margin: 3px 0 3px 0;
}
</style>
