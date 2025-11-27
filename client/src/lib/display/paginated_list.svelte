<script>
	import { tick } from 'svelte';

	let {items = $bindable([]), index = $bindable(0),
		range = 30, advance = 10, reversed = false,
		item_dom_id_prefix,
		render_item, load_items,
		loading_text = "Loading...", to_latest_text = "To latest"
		} = $props();
	let reverse_sign = $derived(reversed ? -1 : 1);

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

	let anchor_id = -1;
	let anchor_top_before;
	let list_scroll_top_before;

	const get_anchor = (id) => {
		for(const el of list_div.getElementsByTagName("*"))
			if(el.id === item_dom_id_prefix + id)
				return el;
	};

	export function rerender(_then, keep_pos=true){
		list_div_scroll_top = list_div.scrollTop;
		is_loading = true;
		load_items(index, range, (list) => {
			if(list.length < range && index > 0){
				is_loading = false;
				index -= (range - list.length);
				rerender(_then);
				return;
			}

			let anchor;
			if(keep_pos){
				anchor_id = items[Math.floor(items.length / 2)].id;
				anchor = get_anchor(anchor_id);
				anchor_top_before = anchor.offsetTop;
				list_scroll_top_before = list_div.scrollTop;
			}

			items = list;
			tick().then(() => {
				if(keep_pos && anchor_id > -1){
					let anchor = get_anchor(anchor_id);
					list_div.scrollTop = list_scroll_top_before + (anchor.offsetTop - anchor_top_before);
					list_div_scroll_top = list_div.scrollTop;
				}

				if(_then)
					_then();
			});
			is_loading = false;
		});
	};

	const on_scroll = (e) => {
		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		if(reverse_sign * next_scroll_top >= max_scroll || reverse_sign * next_scroll_top < 0){
			let dir = Math.sign(e.deltaY);

			if(dir !== 0 && index + reverse_sign * dir >= 0){
				if(is_loading)
					e.preventDefault();
				else{
					index += reverse_sign * dir * advance;
					if(index < 0) index = 0;
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
	// show "Go to latest" button if scrolled more than half-page above last element
	let show_goto_latest = $derived(typeof to_latest_text !== "undefined" &&
		(index > 0 || 
		(typeof list_div !== "undefined" && (reverse_sign * list_div_scroll_top > (list_div.scrollHeight - list_div.clientHeight) * 0.5))
		));
 
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
		}, false);
	};
</script>

<div class="paginated_list">
	<div class="paginated_list" style={reversed ? "flex-direction: column-reverse" : ""} onwheel={on_scroll} bind:this={list_div}>
		{#each items as item, i}
			{@render render_item(i, item)}
		{/each}
	</div>
	{#if is_loading}
		<div class="item paginated_list_overlay">
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 20px; margin-right: 8px"/>
			<span class="paginated_list_overlay_text">{loading_text}</span>
		</div>
	{/if}
	{#if show_goto_latest}
		<button class="item paginated_list_overlay paginated_list_overlay_tolatest" onclick={goto_latest}>
			<span class="paginated_list_overlay_text" style="text-decoration: underline">{to_latest_text}</span>
		</button>
	{/if}
</div>

<style>
.paginated_list {
	min-height: 42px;
	max-height: 100%;

	overflow-y: hidden;
	display: flex;
	flex-direction: column;
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
