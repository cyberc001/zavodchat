<script>
	import {asset} from '$app/paths';
	import {onDestroy} from 'svelte';

	import {VideoState} from '$lib/socket/vc_utils.svelte.js';

	import IconButton from '$lib/control/icon_button.svelte';
	import Sound from '$lib/sound.js';

	import User from '$lib/rest/user.svelte.js';


	const {socket_vc, end_call} = $props();

	let self_user = User.get(-1);

	let other_user = $state();
	$effect(() => {
		if(socket_vc.get_channel()?.loaded && typeof(socket_vc.get_channel().data.other_user_id) !== "undefined")
			other_user = User.get(socket_vc.get_channel().data.other_user_id);
		else
			other_user = undefined;
	});

	$effect(() => {
		// If self user is in the call, but the other user isnt, it's an outgoing call and ringtone should play
		if(self_user.loaded && other_user?.loaded &&
			socket_vc.has_user(self_user.data.id) && !socket_vc.has_user(other_user.data.id))
			Sound.play_ringtone(asset("sounds/out_call.ogg"));
		else
			Sound.stop_ringtone();
	});
	onDestroy(() => {
		Sound.stop_ringtone();
	});

	let name = $derived(other_user ? other_user.data.name : socket_vc.get_channel()?.data?.name);
</script>

<div class="panel vc_panel" style="border-bottom: none">
	<div style="display: flex; align-items: center; margin-bottom: 6px">
		<div class="channel_name text_ellipsis">
			{name}
		</div>

		<div style="margin-left: auto; display: flex">
			<IconButton icon={asset("icons/screen_share" + (socket_vc.get_video_state() === VideoState.Screen ? "_stop" : "") + ".svg")}
				onclick={() => socket_vc.set_video_state(socket_vc.get_video_state() === VideoState.Disabled ?
										VideoState.Screen : VideoState.Disabled)}
			/>
			<IconButton icon={asset("/icons/" + (socket_vc.is_mute() ? "" : "not_") + "muted.svg")}
				onclick={() => socket_vc.toggle_mute()}
			/>
			<IconButton icon={asset("icons/" + (socket_vc.is_deaf() ? "" : "not_") + "deaf.svg")}
				onclick={() => socket_vc.toggle_deaf()}
			/>
			<IconButton icon={asset("icons/hang.svg")}
				onclick={end_call}
			/>
		</div>
	</div>
	<div style="color: {socket_vc.get_state().color}">
		{socket_vc.get_state().text}
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

.channel_name {
	margin-right: 4px;
}
</style>
