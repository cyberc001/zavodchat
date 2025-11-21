<script>
	import { tick } from 'svelte';

	let {items = $bindable([]), index = $bindable(0),
		range = 20, advance = 2,
		render_item, load_items,
		} = $props();

	let is_loading = $state(true);
	load_items(index, range, (list) => {
		items = list;
		is_loading = false;
	});

	let last_scroll_top;
	const is_scrolling_up = (scroll_top) => {
		if(typeof last_scroll_top === "undefined"){
			last_scroll_top = scroll_top;
			return;
		}
		let ret = scroll_top < last_scroll_top;
		last_scroll_top = scroll_top;
		return ret;
	};

	let list_div = $state();

	let anchor_id;
	let anchor_top_before;
	let list_scroll_top_before;

	export function rerender(_then){
		list_div_scroll_top = list_div.scrollTop;
		is_loading = true;
		load_items(index, range, (list) => {
			if(list.length < range){
				index += dir * advance;
				is_loading = false;
				return;
			}

			anchor_id = items[Math.floor(items.length / 2)].id;
			let anchor = document.getElementById("message_display_" + anchor_id);
			anchor_top_before = anchor.offsetTop;
			list_scroll_top_before = list_div.scrollTop;
			items = list;
			tick().then(() => {
				let anchor = document.getElementById("message_display_" + anchor_id);
				list_div.scrollTop = list_scroll_top_before + (anchor.offsetTop - anchor_top_before);
				list_div_scroll_top = list_div.scrollTop;
				if(_then)
					_then();
			});
			is_loading = false;
		});
	};

	const on_scroll = (e) => {
		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		if(-next_scroll_top >= max_scroll || next_scroll_top >= 0){
			let dir = Math.sign(e.deltaY);

			if(dir !== 0 && index - dir >= 0){
				if(is_loading)
					e.preventDefault();
				else{
					index -= dir * advance;
					list_div.scrollTop = next_scroll_top;
					rerender();
				}
			} else
				list_div.scrollTop = next_scroll_top;
		} else 
			list_div.scrollTop = next_scroll_top;
		list_div_scroll_top = list_div.scrollTop;
	};

	let list_div_scroll_top = $state(0);
	// show "Go to latest" button if scrolled more than half-page above latest message
	let show_goto_latest = $derived(index > 0 || (typeof list_div !== "undefined" && (-list_div_scroll_top > (list_div.scrollHeight - list_div.clientHeight) * 0.5)));
 
	const goto_latest = () => {
		if(index == 0){
			list_div.scrollTop = 0;
			list_div_scroll_top = list_div.scrollTop;
			return;
		}

		index = 0;
		rerender(() => {
			list_div.scrollTop = 0;
			list_div_scroll_top = list_div.scrollTop;
		});
	};
</script>

<div class="paginated_list">
	<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
		{#each items as item, i}
			{@render render_item(i, item)}
		{/each}
	</div>
	{#if is_loading}
		<div class="item paginated_list_overlay">
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 20px; margin-right: 8px"/>
			<span class="paginated_list_overlay_text">Loading messages...</span>
		</div>
	{/if}
	{#if show_goto_latest}
		<button class="item paginated_list_overlay paginated_list_overlay_tolatest" onclick={goto_latest}>
			<span class="paginated_list_overlay_text" style="text-decoration: underline">To latest messages</span>
		</button>
	{/if}
</div>

<style>
.paginated_list {
	min-height: 42px;

	overflow-y: hidden;
	display: flex;
	flex-direction: column-reverse;
	position: relative;

	overflow-anchor: none;
}
.paginated_list_overlay {
	display: flex;
	align-items: center;
	justify-content: center;

	padding: 0 8px 0 8px;
	height: 28px;

	position: absolute;
	top: 0%;
	left: 50%;
	transform: translate(-50%, 0%);
}
.paginated_list_overlay_tolatest {
	top: auto;
	bottom: 0%;
	border-style: none;
}
.paginated_list_overlay_text {
	font-size: 20px;
	color: var(--clr_text);
}
</style>
