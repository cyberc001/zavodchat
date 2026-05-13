<script>
	import {asset} from '$app/paths';
	
	import Util from '$lib/util.js';
	import VCSocket from '$lib/socket/vc.svelte';
	import {VideoState} from '$lib/socket/vc_utils.svelte.js';

	import Button from '$lib/control/button.svelte';

	import User from '$lib/rest/user.svelte.js';

	const {
		socket_vc,
		end_call, ctx_vc_user
	} = $props();

	let self = $state();

	let self_user = User.get(-1);

	let vc_users = $derived.by(() => {
		if(!socket_vc?.get_channel()?.data.vc_users)
			return [];
		let users = Object.values(socket_vc.get_channel().data.vc_users);
		if(typeof(socket_vc.get_channel().data.other_user_id) !== "undefined" && users.length < 2){
			// Trying to call the other user; add a dummy voice state
			users.push({
				user: User.get(socket_vc.get_channel().data.other_user_id)
			});
		}
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
				role="figure"
			>
			{#if vc_state?.user?.loaded}
				<img class="user_avatar" src={User.get_avatar_path(vc_state.user.data)}
					style={"border-color: #00FF00"
						+ (socket_vc?.get_audio(vc_state.user.data.id) ?
							Util.padded_hex(Math.min(socket_vc.get_audio(vc_state.user.data.id).amplitude / 10, 1) * 255)
							: "00")
						+ (typeof(vc_state.id) === "undefined" ? ";filter: brightness(40%)" : "")
					}
					alt="avatar"
				/>
				<b class="user_name text_ellipsis">{vc_state.user.data.name}</b>

				<div class="vc_state">
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
			{:else}
				<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 32px"/>
			{/if}
			</div>
		{/each}
	</div>

	<div class="actions">
		<Button icon={asset("icons/screen_share" + (socket_vc.get_video_state() === VideoState.Screen ? "_stop" : "") + ".svg")}
			onclick={() => {
				socket_vc.set_video_state(socket_vc.get_video_state() === VideoState.Disabled ?
								VideoState.Screen : VideoState.Disabled);
			}}
			--margin="0 4px 0 0"
			--padding="2px"
			--display="flex"
		/>
		<Button icon={asset("/icons/" + (socket_vc.is_mute() ? "" : "not_") + "muted.svg")}
			onclick={() => {
				socket_vc.toggle_mute();
			}}
			--margin="0 4px 0 0"
			--padding="2px"
			--display="flex"
		/>
		<Button icon={asset("icons/" + (socket_vc.is_deaf() ? "" : "not_") + "deaf.svg")}
			onclick={() => {
				socket_vc.toggle_deaf();
			}}
			--margin="0 4px 0 0"
			--padding="2px"
			--display="flex"
		/>
		<Button icon={asset("icons/hang.svg")}
			onclick={end_call}
			--padding="2px"
			--display="flex"
			--background="var(--clr_bg_item_negative)"
			--border-color="var(--clr_border_item_negative)"
		/>
	</div>
</div>
{/if}

<style>
.private_call_container {
	display: grid;
	flex-direction: column;
	justify-items: center;

	anchor-name: --private_call_container;
}

.private_call_users {
	display: flex;
	justify-content: center;
}
.private_call_user {
	width: min(150px, 25vw);
	margin: 8px 0 4px 0;

	align-items: center;
	display: flex;
	flex-direction: column;
}
.private_call_user:not(:last-child) {
	margin-right: min(20%, 50px);
}

.user_name {
	width: 100%;
	font-size: 18px;
	text-align: center;
}
.user_avatar {
	width: min(150px, 25vw);
	aspect-ratio: 1;

	border-style: solid;
	border-width: 4px;
	border-radius: 10px;
}

.vc_state {
	display: flex;
	align-items: center;
	justify-content: center;
	min-height: 20px;
}
.vc_state_icon {
	height: 18px;
}

.actions {
	width: fit-content;
	display: flex;
	margin-bottom: 4px;
}
</style>
