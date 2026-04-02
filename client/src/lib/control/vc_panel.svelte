<script>
	import {asset} from '$app/paths';
	import {onDestroy} from 'svelte';

	import VCSocket from '$lib/socket/vc.svelte.js';

	import Sound from '$lib/sound.js';

	import User from '$lib/rest/user.svelte.js';


	const {socket_vc, end_call} = $props();

	let self_user = User.get(-1);

	let other_user = $state();
	$effect(() => {
		if(socket_vc.channel.loaded && typeof(socket_vc.channel.data.other_user_id) !== "undefined")
			other_user = User.get(socket_vc.channel.data.other_user_id);
		else
			other_user = undefined;
	});

	$effect(() => {
		// If self user is in the call, but the other user isnt, it's an outgoing call and ringtone should play
		if(self_user.loaded && other_user?.loaded && socket_vc &&
			socket_vc.has_user(self_user.data.id) && !socket_vc.has_user(other_user.data.id))
			Sound.play_ringtone(asset("sounds/out_call.ogg"));
		else
			Sound.stop_ringtone();
	});
	onDestroy(() => {
		Sound.stop_ringtone();
	});

	let name = $derived(other_user ? other_user.data.name : socket_vc.channel.data.name);
</script>

<div class="panel vc_panel" style="border-bottom: none">
	<div style="display: flex; align-items: center; margin-bottom: 6px">
		{name}

		<div style="margin-left: auto; display: flex">
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
	<div style="color: {socket_vc.state.color}">
		{socket_vc.state.text}
	</div>
</div>

<style>
.vc_panel {
	border-style: solid;

	box-sizing: border-box;
	height: auto;

	width: 100%;
	padding: 8px;

	font-size: 18px;
	overflow-wrap: anywhere;
}
</style>
