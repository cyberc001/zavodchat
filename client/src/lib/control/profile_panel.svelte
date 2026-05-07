<script>
	import {asset} from '$app/paths';

	import IconButton from '$lib/control/icon_button.svelte';
	import UserDisplay from '$lib/display/user.svelte';

	import User from '$lib/rest/user.svelte.js';
	import Status from '$lib/rest/status.js';


	let {show_ctx_menu} = $props();

	let user_self = User.get(-1);
</script>


{#snippet status(hide_ctx_menu, status_enum, status_name, status_clr)}
	<button class="transparent_button unhoverable status_button"
		onclick={() => {
			Status.change(status_enum, () => {});
			hide_ctx_menu();
		}}>
		<div class="user_role_circle_big status_circle" style="background: {status_clr}"></div>
		{status_name}
	</button>
{/snippet}

{#snippet status_online(hide_ctx_menu)}
	{@render status(hide_ctx_menu, Status.Online, "Online", "var(--clr_online)")}
{/snippet}
{#snippet status_away(hide_ctx_menu)}
	{@render status(hide_ctx_menu, Status.Away, "Away", "var(--clr_away)")}
{/snippet}
{#snippet status_donotdisturb(hide_ctx_menu)}
	{@render status(hide_ctx_menu, Status.DoNotDisturb, "Do not disturb", "var(--clr_donotdisturb)")}
{/snippet}
{#snippet status_offline(hide_ctx_menu)}
	{@render status(hide_ctx_menu, Status.Offline, "Offline", "var(--clr_offline)")}
{/snippet}


<div class="panel profile_panel">
	{#if !user_self.loaded}
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 24px"/>
	{:else}
		<UserDisplay id="profile_user_display"
			user={user_self} 
			show_user={(id, e, anchor) => {
				const rect = anchor.getBoundingClientRect();
				show_ctx_menu(anchor, e,
						[status_online, status_away, status_donotdisturb, status_offline],
						[rect.width + 10, 0]);
			}}
		/>
	{/if}
</div>

<style>
.profile_panel {
	border-style: solid;

	box-sizing: border-box;
	height: auto;

	width: 100%;
	padding: 8px;

	font-size: 18px;
	overflow-wrap: anywhere;
}

.status_button {
	width: 100%;
	font-size: 20px;
	display: flex;
	align-items: baseline;
	padding: 0 2px 0 2px;
}
.status_circle {
	margin-right: 4px;
}
</style>
