<script>
	import {onDestroy} from 'svelte';

	import FocusManager from '$lib/focus_manager.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import List from '$lib/control/list.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	let {
		label_text = "",

		render_data, get_data, prepended_data,
		get_data_name = (x) => x.name,

		on_picked = () => {}, 

		list_on_top = false, // display the list above input field
		fixed_text_color = false, // dont change text color to secondary when user wasnt picked

		override_value_name, // if it's not undefined, input is hidden
		value = $bindable()
	} = $props();

	let self = $state();
	let input = $state();
	let paginated_list = $state();

	let list_data = $derived.by(() => {
		let res = [];
		if(typeof(prepended_data) === "function")
			res = prepended_data(value_name);
		else if(typeof(prepended_data) !== "undefined")
			res = prepended_data.filter((x) => get_data_name(x).startsWith(value_name));

		// Limit output to 50 items
		return res.slice(0, 50);
	});

	let show_list = $state(false);
	let list_style = $derived.by(() => {
		let style = list_on_top ? "bottom: 100%;" : `top: calc(100% - ${getComputedStyle(input).marginBottom});`;

		// If paginated items are loading, or both lists are empty, hide the panel
		if(paginated_list?.isLoaded() === false
			|| ((!paginated_list || paginated_list.getItemCount() === 0) &&
			list_data.length === 0))
			style += "display: none;";
		return style;
	});

	let _value_name = $state("");
	let value_name = $derived(typeof(override_value_name) === "undefined" ? _value_name : override_value_name);
	let prev_value_name = $state("");

	let list_value_name_ts = 0;
	let list_value_name = $state("");
	const list_value_name_intv = setInterval(() => {
		if(list_value_name === value_name || new Date() - list_value_name_ts < 500)
			return;
		list_value_name = value_name;
		list_value_name_ts = new Date();
		if(paginated_list)
			paginated_list.reset();
	}, 100);
	onDestroy(() => clearInterval(list_value_name_intv));

	const pick_value = (item) => {
		value = item;
		_value_name = get_data_name(item);
		show_list = false;

		on_picked(item);
	};

	export function reset(){
		value = undefined;
		list_value_name_ts = 0;
		list_value_name = "";
		_value_name = "";
		if(paginated_list)
			paginated_list.reset();
	}
	export function focus(){
		if(input)
			input.focus();
		show_list = true;
	}
	export function get_first_result(){
		if(list_data.length > 0)
			return list_data[0];
		else if(paginated_list && paginated_list.getItemCount() > 0)
			return paginated_list.getItem(0);
	}
</script>


<!--selectionchange event fires in-between mousedown and mouseup. To make Autocomplete not unfocus after it got focused by MessageInput, mousedown event is used-->
<FocusManager element={self}
	blur_on_mousedown=true 
	onblur={() => {
		show_list = false;
	}}
/>

{#snippet render_list_entry(i, item)}
<button class="transparent_button hoverable" style="width: 100%"
onclick={() => pick_value(item)}
>
	{@render render_data(i, item)}
</button>
{/snippet}

<div bind:this={self} class="autocomplete_frame">
	{#if typeof(override_value_name) === "undefined"}
	<p class="settings_control_label">{label_text}</p>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control"
		style={"width:var(--width, 200px);" +
			(typeof value === "undefined" && !fixed_text_color ? "; color: var(--clr_text_secondary)" : "")
		}
		bind:value={_value_name} bind:this={input}
		onfocus={() => {show_list = true;}}
		onkeyup={(e) => {
			show_list = true;

			if(e.key === "Enter"){
				const res = get_first_result();
				if(typeof(res) !== "undefined")
					pick_value(res);
			} else if(_value_name !== prev_value_name){
				value = undefined;
				prev_value_name = value_name;
			}
		}}
	/>
	{/if}

	{#if show_list}
		<div class="paginated_list_panel item"
			style={list_style}>
			{#if get_data}
			<PaginatedList
				render_item={render_list_entry}
				load_items={(index, range, asc) => get_data(index, range, asc, value_name)}
				prepend_items={list_data}
				bind:this={paginated_list}
				to_latest_text="Up"
				auto_height=true
				--max-height="400px"
			/>
			{:else}
			<List
				items={list_data}
				render_item={render_list_entry}
			/>
			{/if}
		</div>
	{/if}
</div>

<style>
@import "style.css";

.autocomplete_frame {
	display: flex;
	flex-direction: column;

	position: relative;
	z-index: 10;
}

.paginated_list_panel {
	z-index: 1000;
	position: absolute;

	border-style: solid;
	border-radius: 4px;
}
.role_panel {
	width: 100%;
	margin-bottom: 6px;

	text-align: left;
	border-style: none;
	background: transparent;
	color: var(--clr_text);
	font-size: 16px;
}
</style>
