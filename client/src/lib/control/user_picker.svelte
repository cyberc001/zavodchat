<script>
	import {onDestroy} from 'svelte';

	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	
	import FocusManager from '$lib/focus_manager.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	let {server_id, // if undefined, regular users are searched
		prepended_roles = [],
		list_on_top = false, // display the list above input field
		exit_input = () => {}, user_picked = () => {}, role_picked = () => {},
		value = $bindable()} = $props();

	let self = $state();
	let input = $state();
	let user_list = $state();

	let roles = $derived(prepended_roles.filter((x) => x.name.indexOf(user_name) !== -1));

	let show_list = $state(false);
	let list_style = $derived.by(() => {
		let style = "";
		if(list_on_top)
			style += "bottom: 100%;";
		if(user_list && user_list.getItemCount() === 0 && (roles.length === 0 || !user_list.isLoaded()))
			style += "display: none;";
		return style;
	});

	let user_name = $state("");
	let prev_user_name = $state("");

	let list_user_name_ts = 0;
	let list_user_name = $state("");
	const list_user_name_intv = setInterval(() => {
		if(list_user_name === user_name || new Date() - list_user_name_ts < 500)
			return;
		list_user_name = user_name;
		list_user_name_ts = new Date();
		if(user_list)
			user_list.reset();
	}, 100);
	onDestroy(() => clearInterval(list_user_name_intv));


	const load_items = (index, range, asc) => server_id ? User.get_server_range(server_id, index, range, asc, list_user_name)
							    : User.get_range(index, range, asc, list_user_name);

	export function reset(){
		value = undefined;
		list_user_name_ts = 0;
		list_user_name = "";
		user_name = "";
		if(user_list)
			user_list.reset();
	}
	export function focus(){
		input.focus();
		show_list = true;
	}
</script>


<!--selectionchange event fires in-between mousedown and mouseup. To make UserPicker not unfocus after it got focused by MessageInput, mousedown event is used-->
<FocusManager element={self}
	blur_on_mousedown=true 
	onblur={() => {
		show_list = false;
	}}
/>

{#snippet render_role(i, item)}
	<button class="role_panel item hoverable"
	style={Role.get_color_style(item)}
	onclick={() => role_picked(item.id)}
	>
		{item.name}
	</button>
{/snippet}
{#snippet render_user(i, item)}
	<UserDisplay user={item} display_status={false}
		show_user={() => {
			value = item.id;
			user_name = user_list.getItem(i).name;
			show_list = false;
			user_picked(item.id);
		}}
	/>
{/snippet}

<div style="position: relative;" bind:this={self}>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control"
		style={"width:var(--width, 200px); margin-bottom: var(--margin-bottom, 12px)" +
			(typeof value === "undefined" ? "; color: var(--clr_text_secondary)" : "")
		}
		bind:value={user_name} bind:this={input}
		onfocus={() => {show_list = true;}}
		onkeyup={(e) => {
			if(e.key === "Backspace" && !prev_user_name.length)
				exit_input();

			if(user_name !== prev_user_name){
				value = undefined;
				prev_user_name = user_name;
			}
		}}
	/>

	{#if show_list}
		<div class="user_list_panel item"
			style={list_style}>
			<PaginatedList
				render_item={render_user} load_items={load_items}
				render_prepend_item={render_role} prepend_items={roles}
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
.role_panel {
	margin-bottom: 6px;

	text-align: left;
	border-style: none;
	background: transparent;
	color: var(--clr_text);
	font-size: 20px;
}
</style>
