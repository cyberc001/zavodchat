<script>
	import { tick } from 'svelte';

	let {items, index = $bindable(0), range = 20,
		render_item, load_items} = $props();

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

	let anchor_top_before = 0;
	let list_scroll_top_before = 0;

	let is_loading = false;
	const on_scroll = (e) => {
		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.1;

		if(-next_scroll_top >= max_scroll || next_scroll_top >= 0){
			let dir = Math.sign(e.deltaY);

			if(dir !== 0 && index - dir >= 0){
				if(is_loading){
					e.preventDefault();
				}
				else{
					is_loading = true;
					index -= dir;
					let scroll_delta_on_load = next_scroll_top - list_div.scrollTop;
					load_items(index, range, (list) => {
						if(list.length < range){
							index += dir;
							is_loading = false;
							return;
						}

						let anchor_id = items[dir == -1 ? items.length - 1 : 0].id;
						let anchor = document.getElementById("message_display_" + anchor_id);
						anchor_top_before = anchor.offsetTop;
						list_scroll_top_before = list_div.scrollTop;
						items = list;
						tick().then(() => {
							let anchor = document.getElementById("message_display_" + anchor_id);
							list_div.scrollTop = list_scroll_top_before + (anchor.offsetTop - anchor_top_before) + scroll_delta_on_load;
						});
						is_loading = false;
					});
				}
			}
		} else 
			list_div.scrollTop = next_scroll_top;
	};
</script>

<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
	{#each items as item}
		{@render render_item(item)}
	{/each}
</div>

<style>
.paginated_list {
	overflow-y: hidden;
	display: flex;
	flex-direction: column-reverse;

	overflow-anchor: none;
}
</style>
