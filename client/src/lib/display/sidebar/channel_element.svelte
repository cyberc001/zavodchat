<script>
	import {asset} from '$app/paths';
	import Util from '$lib/util.js';
	import Channel from '$lib/rest/channel.js';
	import User from '$lib/rest/user.svelte.js';	

	import VCSocket from '$lib/socket/vc.svelte.js';

	const {selected, last,
		channel, socket_vc,
		show_channel, ctx_channel, ctx_vc_user} = $props();

	let self = $state();
	let vc_user_divs = $state({});
</script>

<div>
	<button class={"item hoverable transparent_button sidebar_channel_el" + (selected ? " selected" : "")}
		style={(last ? "border-style: solid none solid none" : "")
			+ "; anchor-name: --channel_" + channel.id
			+ "; padding: 4px 0px 4px 4px"}
		onclick={() => show_channel(channel.id)}
		bind:this={self}
		oncontextmenu={(e) => {
			event.preventDefault();
			ctx_channel(self, e, channel);
		}}
	>
		{#if channel.type === Channel.Type.Voice}
			<img src={asset("icons/channel_vc.svg")} alt="voice_channel" class="filter_icon_main sidebar_channel_el_icon"/>
		{:else}
			<img src={asset("icons/channel_text.svg")}  alt="text_channel" class="filter_icon_main sidebar_channel_el_icon"/>
		{/if}
		{channel.name}
	</button>
	{#if channel.type === Channel.Type.Voice && typeof channel.vc_users === "object"}
		{#each Object.values(channel.vc_users) as vc_state}
			<div
				style="display: flex; align-items: center; margin: 3px 0 3px 6px; font-size: 22px; anchor-name: --{"vc_user_" + vc_state.id}"

				bind:this={vc_user_divs[vc_state.id]}
				oncontextmenu={(e) => {
					event.preventDefault();
					ctx_vc_user(self, e, vc_state);
				}}
			>
				<img src={User.get_avatar_path(vc_state.user.data)}
					alt="avatar"
					style={"width: 32px; height: 32px; margin-right: 8px; border-style: solid; border-size: 2px; border-color: #00FF00"
						+ (socket_vc && socket_vc.audio[vc_state.user.data.id] ?
							Util.padded_hex(Math.min(socket_vc.audio[vc_state.user.data.id].amplitude / 10, 1) * 255)
							: "00")
					}
				/>
				{vc_state.user.data.name}
				<div style="margin-left: auto; margin-right: 4px; display: flex; align-items: center">
					{#if vc_state.video > VCSocket.VideoState.Disabled}
						<div style="background: red; border-radius: 4px; font-size: 18px; padding: 0 3px 0 3px; margin-right: 3px; display: inline-block">
							STREAM
						</div>
					{/if}
					{#if socket_vc && socket_vc.video[vc_state.user.data.id]}
					<button class="hoverable transparent_button"
							onclick={() => {
								socket_vc.watch_video(vc_state.user.data.id);
							}}
					>
						<img src={asset("icons/watch.svg")} alt="watch" class="filter_icon_main vc_state_icon"/>
					</button>
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
</style>
