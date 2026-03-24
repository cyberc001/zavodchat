<script>
	import FocusManager from '$lib/focus_manager.svelte';
	import User from '$lib/rest/user.svelte.js';
	import UserDisplay from '$lib/display/user.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	let {server_id, // if undefined, regular users are searched
		value = $bindable()} = $props();

	let self = $state();
	let user_list = $state();
	let show_list = $state(false);

	let user_name = $state("");
	let prev_user_name = $state("");
	$effect(() => {
		user_name;
		user_list?.reset();
	});

	export function reset(){
		value = undefined;
		user_name = "";
	}
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

<div style="position: relative;" bind:this={self}>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control"
		style={"width:var(--width, 200px); margin-bottom: var(--margin-bottom, 12px)" +
			(typeof value === "undefined" ? "; color: var(--clr_text_secondary)" : "")
		}
		bind:value={user_name}
		onfocus={() => {show_list = true;}}
		onkeyup={() => {
			if(user_name !== prev_user_name){
				value = undefined;
				prev_user_name = user_name;
			}
		}}
	/>

	{#if show_list}
		<div class="user_list_panel item"
			style={user_list && user_list.getItemCount() > 0 ? "" : "display: none"}>
			<PaginatedList
				render_item={render_user}
				load_items={(index, range, asc) => server_id ? User.get_server_range(server_id, index, range, asc, user_name)
										: User.get_range(index, range, asc, user_name)
				}
				bind:this={user_list}
				to_latest_text="Up"
				auto_height=true
				--width="200px"
				--max-height="400px"
			/>
		</div>
	{/if}
</div>

<style>
@import "style.css";

.user_list_panel {
	z-index: 1000;
	position: absolute;
	border-style: solid;
	border-radius: 4px;
}
</style>
