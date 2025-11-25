<script>
	import User from '$lib/rest/user.js';
	let {user, display_status = true, div_classes = ""} = $props();

	let status_style = $derived.by(() => {
		if(!display_status)
			return "";
		let var_name;
		switch(user?.status){
			case User.Status.Offline:
				var_name = "clr_offline";
				break;
			case User.Status.Online:
				var_name = "clr_online";
				break;
			case User.Status.Away:
				var_name = "clr_away";
				break;
			case User.Status.DoNotDisturb:
				var_name = "clr_donotdisturb";
				break;
		}
		return typeof var_name === "undefined" ? "" : `background: var(--${var_name})`;
	});
</script>

<div class={"user_display " + div_classes} id={user ? "user_display_" + user.id : ""}>
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
.user_display {
	display: flex;
	align-items: center;

	margin-bottom: 6px;

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
