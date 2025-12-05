<script>
	import User from '$lib/rest/user.js';
	import Role from '$lib/rest/role.js';

	// rel_off - offset relative to size, in %
	let {user, user_roles,
		pos = [0, 0], rel_off = [0, 0],
		hide_profile} = $props();

	let self = $state();

	const pos_margin_top = 14, pos_margin_bottom = 64;
	let pos_off = $derived.by(() => {
		pos;
		let po = [0, 0];
		if(typeof self !== "undefined"){
			let brect = self.getBoundingClientRect();
			if(pos[1] + brect.height > document.documentElement.clientHeight - pos_margin_bottom)
				po[1] = document.documentElement.clientHeight - pos_margin_bottom - (pos[1] + brect.height);
			if(pos[1] < pos_margin_top)
				po[1] = pos_margin_top - pos[1];
		}
		return po;
	});
	$effect(() => {
		if(typeof self !== "undefined"){
			let brect = self.getBoundingClientRect();
			// close profile if scrolled too far
			if(Math.abs(pos_off[1]) >= brect.height)
				hide_profile();
		}
	});

	let pointer_on_profile = false;
	const onmouseup = () => {
		if(pointer_on_profile)
			return;
		hide_profile();
	};
</script>

<svelte:window {onmouseup}/>
<div class="item user_profile_display" style="left: {pos[0] + pos_off[0]}px; top: {pos[1] + pos_off[1]}px; transform: translate({rel_off[0]}%, {rel_off[1]}%);"
	tabindex=0
	role="dialog"
	onmouseenter={() => pointer_on_profile = true}
	onmouseleave={() => pointer_on_profile = false}
	bind:this={self}
>
	{#if Object.keys(user).length === 0}
		<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
	{:else}
		<div class="user_profile_name">
			<div class="user_avatar_frame">
				<div class="user_status" style={User.Status.get_style(user?.status)}></div>
				{#if user?.avatar}
					<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
				{:else}
					<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
				{/if}
			</div>
			<b style={Role.get_username_style(user_roles)}>{user?.name}</b>
		</div>
		<div class="user_role_list">
			{#each user_roles as rol}
				<div class="user_role">
					<div class="user_role_color" style={Role.get_style(rol)}></div>
					{rol.name}
				</div>
			{/each}
		</div>
	{/if}
</div>

<style>
.user_profile_display {
	position: absolute;
	z-index: 100;

	padding: 4px 6px 4px 6px;
}
.user_profile_name {
	display: flex;
	align-items: center;

	margin-bottom: 6px;

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

.user_role_list {
	width: 177px; /* FIXME */
	display: flex;
	flex-wrap: wrap;
}
.user_role {
	border-style: solid;
	border-radius: 6px;
	border-color: var(--clr_border_item);

	padding: 2px 4px 2px 4px;
	margin: 3px 6px 3px 6px;
}
.user_role_color {
	display: inline-block;
	width: 8px;
	height: 8px;
	background: yellow;
	border-radius: 50%;
}
</style>
