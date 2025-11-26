<script>
	import User from '$lib/rest/user.js';

	let {user, pos,
		hide_profile} = $props();

	let status_style = $derived(User.Status.getStyle(user?.status));

	let pointer_on_profile = false;
	const onmouseup = () => {
		if(pointer_on_profile)
			return;
		hide_profile();
	};
</script>

<svelte:window {onmouseup}/>
<div class="item user_profile_display" style="top: {pos[0]}px; left: {pos[1]}px"
	onmouseenter={() => pointer_on_profile = true}
	onmouseleave={() => pointer_on_profile = false}
>
	<div class="user_avatar_frame">
		<div class="user_status" style={status_style}></div>
		{#if user?.avatar}
			<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
		{:else}
			<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
		{/if}
	</div>
	<b>{user?.name}</b>
</div>

<style>
.user_profile_display {
	position: absolute;

	display: flex;
	align-items: center;

	margin-bottom: 6px;
	padding: 4px 6px 4px 6px;

	border-style: none;
	color: var(--clr_text);
	font-size: 24px;
}
.user_avatar_frame {
	position: relative;
	width: 32px;
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
