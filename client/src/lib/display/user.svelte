<script>
	let {user, div_classes = ""} = $props();
</script>

<div class={"user_display " + div_classes} id={user ? "user_display_" + user.id : ""}>
	{#if user?.avatar}
		<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
	{:else}
		<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
	{/if}
	<b>{user?.name}</b>
</div>

<style>
.user_display {
	display: flex;
	align-items: center;

	margin-bottom: 6px;

	font-size: 24px;
}
.user_avatar {
	width: 32px;
	height: 32px;

	border-radius: 4px;
	margin-right: 8px;
}
</style>
