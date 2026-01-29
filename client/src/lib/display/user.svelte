<script>
	import {asset} from '$app/paths';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	let {user, user_roles,
		selected = false,
		onclick = () => {}, show_ctx_menu = () => {},
		display_status = true,
		style = "",
		message_id = -1} = $props();

	let self = $state();
	let name = $derived("user_display_" + user?.id + (message_id > -1 ? "_" + message_id : ""));
</script>

<button class={"user_display hoverable " + (selected ? "selected " : "") + (message_id > -1 ? "" : " sidebar_user_display")}
	style="anchor-name: --{name}; {style}"
	id={name}
	bind:this={self}
	onclick={onclick}
	oncontextmenu={(e) => {
		event.preventDefault();
		show_ctx_menu(self, e);
	}}
>
	{#if !user || Object.keys(user).length === 0}
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 24px"/>
	{:else}
		<div class="user_avatar_frame">
			{#if display_status}
				<div class="user_status" style={User.Status.get_style(user?.status)}></div>
			{/if}
			<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
		</div>
		<b class="user_name_text" style={Role.get_username_style(user_roles)}>{user?.name}</b>
	{/if}
</button>

<style>

.user_display {
	position: relative;
	display: flex;
	align-items: center;
	height: auto;

	margin-bottom: 6px;

	border-style: none;
	background: transparent;
	color: var(--clr_text);
	font-size: 22px;
}
.user_name_text {
	white-space: nowrap;
	overflow-x: hidden;
	text-overflow: ellipsis;
}

.user_avatar_frame {
	position: relative;
	width: 32px;
	min-width: 32px;
	height: 32px;

	border-radius: 4px;
	margin-right: 8px;
}
.user_avatar {
	width: 100%;
	height: 100%;

	border-radius: 4px;
}
.user_status {
	position: absolute;
	top: 0%;
	left: 100%;
	transform: translate(-100%, 0%);

	width: 8px;
	height: 8px;
	border-radius: 2px;
}
</style>
