<script>
	import FocusManager from '$lib/focus_manager.svelte';
	import User from '$lib/rest/user.svelte.js';
	import UserDisplay from '$lib/display/user.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	let {server_id, value = $bindable()} = $props();

	let self = $state();
	let user_list = $state();
	let show_list = $state(false);

	let user_name = $state("");
	$effect(() => {
		user_name;
		user_list?.reset();
	});
</script>


<FocusManager element={self}
	onblur={() => {
		show_list = false;
	}}
/>


{#snippet render_user(i, item)}
	<UserDisplay user={item} display_status={false}
		onclick={() => {
			value = item.id;
			user_name = user_list.getItem(i).name;
			show_list = false;
		}}
	/>
{/snippet}

<div style="position: relative" bind:this={self}>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control" style="width:var(--width, 200px)"
		bind:value={user_name}
		onfocus={() => {show_list = true;}}
		onkeyup={() => {
			value = undefined;
		}}
	/>

	{#if show_list}
		<div class="server_user_list_panel item">
			<PaginatedList
			render_item={render_user}
			load_items={(index, range) => User.get_server_range(server_id, index, range, user_name)}
			bind:this={user_list}
			to_latest_text="Up"
			/>
		</div>
	{/if}
</div>

<style>
@import "style.css";

.server_user_list_panel {
	position: absolute;
	height: 100vh;
	max-height: 400px;
}
</style>
