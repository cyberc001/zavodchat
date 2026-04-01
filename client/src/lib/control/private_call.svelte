<script>
	import {asset} from '$app/paths';
	
	import Util from '$lib/util.js';
	import VCSocket from '$lib/socket/vc.svelte.js';

	import User from '$lib/rest/user.svelte.js';

	const {socket_vc, end_call} = $props();

	$effect(() => {
		console.log("CHANNEL", $state.snapshot(socket_vc?.channel.data));
	});
</script>

{#if typeof(socket_vc?.channel?.data?.other_user_id) !== "undefined" && Object.keys(socket_vc.channel.data.vc_users).length}
<div class="item private_call_container">
	<div class="private_call_users">
		{#each Object.values(socket_vc.channel.data.vc_users) as vc_state}
			<div class="private_call_user">
			{#if vc_state?.user?.loaded}
				<img class="user_avatar" src={User.get_avatar_path(vc_state.user.data)}
					style={"border-color: #00FF00"
						+ (socket_vc && socket_vc.audio[vc_state.user.data.id] ?
							Util.padded_hex(Math.min(socket_vc.audio[vc_state.user.data.id].amplitude / 10, 1) * 255)
							: "00")
					}
				/>
				<b style="text-align: center">{vc_state.user.data.name}</b>
				<div class="vc_state">
					{#if vc_state.video > VCSocket.VideoState.Disabled}
						<div style="background: red; height: 24px; border-radius: 4px; font-size: 18px; padding: 0 3px 0 3px; margin-right: 3px; display: inline-block">
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
			{:else}
				<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 32px"/>
			{/if}
			</div>
		{/each}
	</div>

	<div class="actions">
		<button class="hoverable transparent_button"
			onclick={() => {
				socket_vc.set_video_state(socket_vc.video_state === VCSocket.VideoState.Disabled ?
								VCSocket.VideoState.Screen : VCSocket.VideoState.Disabled);
			}}
		>
			<img src={asset("icons/screen_share" + (socket_vc.video_state === VCSocket.VideoState.Screen ? "_stop" : "") + ".svg")}
			alt={socket_vc.video_state === VCSocket.VideoState.Screen ? "stop sharing screen" : "share screen"} class="filter_icon_main" style="width: 24px">
		</button>

		<button class="hoverable transparent_button"
			onclick={() => {
				socket_vc.toggle_mute();
			}}
		>
			<img src={asset("/icons/" + (socket_vc.mute == VCSocket.AudioState.None ? "not_" : "") + "muted.svg")}
				alt={(socket_vc.mute == VCSocket.AudioState.None ? "" : "un") + "mute"} class="filter_icon_main" style="width: 24px">
		</button>
		<button class="hoverable transparent_button"
			onclick={() => {
				socket_vc.toggle_deaf();
			}}
		>
			<img src={asset("icons/" + (socket_vc.deaf == VCSocket.AudioState.None ? "not_" : "") + "deaf.svg")}
				alt={(socket_vc.deaf == VCSocket.AudioState.None ? "" : "un") + "deafen"} class="filter_icon_main" style="width: 24px">
		</button>
		<button class="hoverable transparent_button"
			onclick={end_call}
		>
			<img src={asset("icons/hang.svg")} alt="end call" class="filter_icon_main" style="width: 24px">
		</button>
	</div>
</div>
{/if}

<style>
.private_call_container {
	text-align: center;
}

.private_call_users {
	display: flex;
	justify-content: center;
}
.private_call_user {
	font-size: 18px;
	align-text: center;

	margin: 10px 0 10px 0;

	display: flex;
	flex-direction: column;
}
.private_call_user:not(:last-child) {
	max-width: 30%;
	margin-right: min(20%, 50px);
}
.user_avatar {
	width: 150px;
	aspect-ratio: 1;

	border-style: solid;
	border-size: 4px;
}
.vc_state {
	display: flex;
	align-items: center;
	justify-content: center;
	min-height: 32px;
}
.vc_state_icon {
	width: 24px;
}

.actions {
}
</style>
