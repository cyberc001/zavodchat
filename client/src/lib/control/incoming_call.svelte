<script>
	import {asset} from '$app/paths';
	import {onDestroy, untrack} from 'svelte';

	import Sound from '$lib/sound.js';

	import Button from '$lib/control/button.svelte';

	import Friends from '$lib/rest/friends.js';
	import Channel from '$lib/rest/channel.js';
	import User from '$lib/rest/user.svelte.js';

	const {socket_vc, show_channel} = $props();

	let self_user = User.get(-1);
	let friends = Friends.get();
	let incoming_call = $state();
	$effect(() => {
		if(friends.loaded && self_user.loaded){
			let new_incoming_call = undefined;
			for(const friend of friends.data){
				if((socket_vc?.get_channel()?.data.other_user_id === friend.user.data.id && socket_vc?.is_connected()) || !friend.vc_users?.length)
					continue;

				const joined = new Date(friend.vc_users[0].joined);
				if(!new_incoming_call || joined < new_incoming_call.joined)
					new_incoming_call = {
						joined,
						user: friend.user,
						channel_id: friend.vc_channel_id
					};
			}
			untrack(() => incoming_call = new_incoming_call);
		}
	});

	let is_playing_ringtone = $derived(incoming_call && incoming_call.user.loaded);
	$effect(() => {
		if(is_playing_ringtone)
			Sound.play_ringtone(asset("sounds/in_call.ogg"));
		else
			Sound.stop_ringtone();
	});

	document.addEventListener("keyup", onkeyup);
	onDestroy(() => {
		document.removeEventListener("keyup", onkeyup);
		Sound.stop_ringtone();
	});
</script>

{#if incoming_call && incoming_call.user.loaded}
	<div class="incall_display panel">
		<img class="incall_avatar" src={User.get_avatar_path(incoming_call.user.data)} />
		<div class="incall_name">{incoming_call.user.data.name}</div>
		<div>Incoming call</div>
		<div class="incall_actions">
			<Button text="Decline" --margin-bottom="0"
			onclick={() => {
				Channel.kick_user(incoming_call.channel_id, incoming_call.user.data.id,
						() => {});
			}}
			/>
			<Button text="Accept" --margin-bottom="0"
			onclick={() => {
				const ch = Channel.get(incoming_call.channel_id);
				ch.notify_on_load(() => {
					show_channel(ch.data);
				});
			}}
			/>
		</div>
	</div>
{/if}

<style>
.incall_display {
	position: fixed;
	left: 50%;
	top: 50%;
	transform: translate(-50%, -50%);
	height: min(300px, 60%);
	width: min(300px, 60%);

	padding: auto;
	align-content: center;
	text-align: center;

	border-style: solid;
	border-width: 2px;
	border-radius: 4px;
	border-color: var(--clr_border);

	z-index: 100;
}

.incall_avatar {
	width: 60%;
	height: 60%;
}
.incall_name {
	font-size: 18px;
	font-weight: bold;
}
.incall_actions {
	display: flex;
	justify-content: center;
	margin-top: 6px;
}
</style>
