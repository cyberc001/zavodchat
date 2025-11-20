<script>
	import { tick } from 'svelte';

	let {items = $bindable("loading"), index = $bindable(0),
		range = 20,
		render_item, load_items,
		} = $props();

	load_items(index, range, (list) => {
		items = list;
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

	let list_div;

	let anchor_id;
	let anchor_top_before;
	let list_scroll_top_before;

	export function rerender(){
		is_loading = true;
		load_items(index, range, (list) => {
			if(list.length < range){
				index += dir;
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
			});
			is_loading = false;
		});
	};

	let is_loading = false;
	const on_scroll = (e) => {
		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		if(-next_scroll_top >= max_scroll || next_scroll_top >= 0){
			let dir = Math.sign(e.deltaY);

			if(dir !== 0 && index - dir >= 0){
				if(is_loading)
					e.preventDefault();
				else{
					index -= dir;
					list_div.scrollTop = next_scroll_top;
					rerender();
				}
			} else
				list_div.scrollTop = next_scroll_top;
		} else 
			list_div.scrollTop = next_scroll_top;
	};
</script>

<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
	{#if items === "loading"}
		<div style="margin: 8px; display: flex; align-items: center">
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 32px; margin-right: 8px"/>
			<span class="paginated_list_message">Loading messages...</span>
		</div>
	{:else}
		{#each items as item, i}
			{@render render_item(i, item)}
		{/each}
	{/if}
</div>

<style>
.paginated_list {
	overflow-y: hidden;
	display: flex;
	flex-direction: column-reverse;

	overflow-anchor: none;
}
.paginated_list_message {
	font-size: 24px;
	color: var(--clr_text);
}
</style>
