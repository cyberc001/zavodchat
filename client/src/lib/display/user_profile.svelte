<script>
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import ContextMenu from '$lib/control/context_menu.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	let {user, server_roles,
		anchor, anchor_side_x = "left",
		hide_profile,
		assign_role, disallow_role} = $props();

	let user_roles = $derived(Role.get_user_roles(user, server_roles));

	let self = $state();

	let pointer_on_profile = false;
	const onmouseup = () => {
		if(pointer_on_profile)
			return;
		hide_profile();
	};

	let add_role_button = $state();
	let add_role_ctx_off = $state([0, 0]);
	let show_add_role_menu = $state(false);
	let add_role_items = $derived.by(() => {
		if(Object.keys(user).length === 0)
			return [];

		let items = [[], []];
		for(const rol of server_roles)
			if(user_roles.findIndex((x) => x.id === rol.id) === -1){
				items[0].push(item_add_role);
				items[1].push(rol);
			}
		return items;
	});
</script>

{#snippet item_add_role(hide_ctx_menu, i)}
	<ContextMenuAction text={add_role_items[1][i].name}
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => assign_role(add_role_items[1][i].id)}
	/>
{/snippet}

<svelte:window {onmouseup}/>
<div class="item user_profile_display"
	style="position-anchor: {getComputedStyle(anchor).getPropertyValue("anchor-name")};
						{anchor_side_x}: anchor(left, 10000px); top: anchor(top, -10000px)"
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
			{#each user_roles as rol, i}
				<button class={"user_role transparent_button hoverable" + (i === user_roles.length - 1 ? "" : " user_role_disallow")}
				disabled={i === user_roles.length - 1}
				onclick={(e) => {
					e.target.blur();
					disallow_role(rol.id);
				}}
				>
					<div class="user_role_color" style={Role.get_background_style(rol)}></div>
					{rol.name}
				</button>
			{/each}
			{#if user_roles.length > 0}
				<button class="user_role transparent_button hoverable"
					bind:this={add_role_button}
					onclick={(e) => {
						const rect = add_role_button.getBoundingClientRect();
						add_role_ctx_off = [e.clientX - rect.left, e.clientY - rect.top];
						show_add_role_menu = true;
					}}
					style="anchor-name: --add_role_button"
				>
					<img class="filter_icon_main role_add_icon" src="$lib/assets/icons/add.svg"> add role
				</button>
			{/if}
		</div>
	{/if}

	{#if show_add_role_menu}
		<ContextMenu anchor={add_role_button} off={add_role_ctx_off}
			hide_ctx_menu={() => show_add_role_menu = false}
			items={add_role_items[0]}
		/>
	{/if}
</div>

<style>
.user_profile_display {
	position: absolute;
	z-index: 10;

	padding: 4px 6px 4px 6px;

	min-width: 160px;
	max-width: 320px;
	width: max-content;
}
.user_profile_name {
	display: flex;
	align-items: center;

	margin-bottom: 6px;

	border-style: none;
	color: var(--clr_text);
	font-size: 24px;

	overflow-wrap: anywhere;
}

.user_avatar_frame {
	position: relative;
	min-width: 32px;
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
	width: 100%;
	display: flex;
	flex-wrap: wrap;
}
.user_role {
	border-style: solid;
	border-radius: 6px;
	border-color: var(--clr_border_item);
	border-width: 3px;

	display: flex;
	align-items: center;

	color: var(--clr_text);

	padding: 2px 4px 2px 4px;
	margin: 3px 6px 3px 0px;
}
.user_role_disallow:hover {
	text-decoration: line-through;
}
.user_role_disallow:focus {
	text-decoration: line-through;
}
.user_role_color {
	display: inline-block;
	width: 8px;
	height: 8px;
	background: yellow;
	border-radius: 50%;

	margin-right: 4px;
}

.role_add_icon {
	width: 12px;
	height: 12px;

	margin-right: 4px;
}
</style>
