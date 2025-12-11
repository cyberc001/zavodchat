<script>
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	let {user, user_roles,
		selected = false, onclick = () => {}, hide_profile,
		display_status = true,
		div_classes = "", create_id = true} = $props();

	let self = $state();
</script>

<button class={"user_display hoverable " + (selected ? "selected " : "") + div_classes}
	id={create_id ? (user ? "user_display_" + user.id : "") : ""}
	onclick={onclick}
	bind:this={self}
>
	{#if !user || Object.keys(user).length === 0}
		<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 24px"/>
	{:else}
		<div class="user_avatar_frame">
			{#if display_status}
				<div class="user_status" style={User.Status.get_style(user?.status)}></div>
			{/if}
			{#if user?.avatar}
				<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
			{:else}
				<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
			{/if}
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
