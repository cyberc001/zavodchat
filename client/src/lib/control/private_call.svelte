<script>
	import {asset} from '$app/paths';
	
	import Util from '$lib/util.js';
	import VCSocket from '$lib/socket/vc.svelte';
	import {VideoState} from '$lib/socket/vc_utils.svelte.js';

	import Button from '$lib/control/button.svelte';

	import User from '$lib/rest/user.svelte.js';

	const {socket_vc,
		end_call, ctx_vc_user} = $props();

	let self = $state();

	let self_user = User.get(-1);

	let vc_users = $derived.by(() => {
		console.log("CHECK", socket_vc?.get_channel());
		if(!socket_vc?.get_channel()?.data.vc_users)
			return [];
		let users = Object.values(socket_vc.get_channel().data.vc_users);
		if(typeof(socket_vc.get_channel().data.other_user_id) !== "undefined" && users.length < 2){
			// Trying to call the other user; add a dummy voice state
			users.push({
				user: User.get(socket_vc.get_channel().data.other_user_id)
			});
		}
		console.log("USERS", users, "\n", $state.snapshot(socket_vc.get_channel().data));
		return users;
	});
</script>

{#if self_user.loaded && typeof(socket_vc?.get_channel()?.data?.other_user_id) !== "undefined" &&
	typeof(socket_vc?.get_channel()?.data?.vc_users[self_user.data.id]) !== "undefined"}
<div class="item private_call_container" bind:this={self}>
	<div class="private_call_users">
		{#each vc_users as vc_state}
			<div class="private_call_user"
				oncontextmenu={(e) => {
					event.preventDefault();
					// Don't allow dummy users
					if(typeof(vc_state.id) === "undefined")
						return;
					ctx_vc_user(self, e, socket_vc.get_channel().data.id, vc_state);
				}}
			>
			{#if vc_state?.user?.loaded}
				<img class="user_avatar" src={User.get_avatar_path(vc_state.user.data)}
					style={"border-color: #00FF00"
						+ (socket_vc?.get_audio(vc_state.user.data.id) ?
							Util.padded_hex(Math.min(socket_vc.get_audio(vc_state.user.data.id).amplitude / 10, 1) * 255)
							: "00")
						+ (typeof(vc_state.id) === "undefined" ? ";filter: brightness(40%)" : "")
					}
				/>
				<b style="text-align: center">{vc_state.user.data.name}</b>

				<div class="vc_state">
					{#if vc_state.video > VideoState.Disabled}
						<div style="background: red; height: 24px; border-radius: 4px; font-size: 18px; padding: 0 3px 0 3px; margin-right: 3px; display: inline-block">
							STREAM
						</div>
					{/if}
					{#if socket_vc?.get_video(vc_state.user.data.id)}
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
		<Button icon={asset("icons/screen_share" + (socket_vc.get_video_state() === VideoState.Screen ? "_stop" : "") + ".svg")}
			--padding-bottom="0px"
			onclick={() => {
				socket_vc.set_video_state(socket_vc.get_video_state() === VideoState.Disabled ?
								VideoState.Screen : VideoState.Disabled);
			}}
		/>
		<Button icon={asset("/icons/" + (socket_vc.is_mute() ? "" : "not_") + "muted.svg")}
			--padding-bottom="0px"
			onclick={() => {
				socket_vc.toggle_mute();
			}}
		/>
		<Button icon={asset("icons/" + (socket_vc.is_deaf() ? "" : "not_") + "deaf.svg")}
			--padding-bottom="0px"
			onclick={() => {
				socket_vc.toggle_deaf();
			}}
		/>
		<Button icon={asset("icons/hang.svg")}
			--padding-bottom="0px"
			onclick={end_call}
		/>
	</div>
</div>
{/if}

<style>
.private_call_container {
	text-align: center;

	anchor-name: --private_call_container;
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
