<script>
	import { tick } from 'svelte';

	let {index = $bindable(0), scrollTop = $bindable(0),
		range = 30, advance = 10, reversed = false,
		item_dom_id_prefix,
		render_item, load_items, augment_item = () => {},
		loading_text = "Loading...", to_latest_text = "To latest"
		} = $props();

	// Keeping this reference is CRUCIAL so that observer does not get garbage collected and items are notified of the changes
	let items_range = $derived(load_items(index, range));

	let items = $derived(items_range.data);
	export function getItem(idx){
		return items[idx];
	}

	// A method for loading additional information into items. Its useful, because:
	// 1. You cannot use cached REST methods in snippets that PaginatedList uses to render items, since it makes PaginatedList mutate static state (i.e. cache) that Main owns
	// 2. Same goes for embedding data directly in REST cached requests using other REST cached requests
	$effect(() => {
		for(let item of items)
			if(item && Object.keys(item).length > 0)
				augment_item(item);
	});

	let keep_scroll_pos = true;
	$effect(() => {
		items;

		let anchor = get_anchor(anchor_id);
		if(anchor && keep_scroll_pos){
			list_div.scrollTop = list_scroll_top_before + (anchor.offsetTop - anchor_top_before);
			remember_scroll_pos();
		}
		keep_scroll_pos = true;
	});

	$effect(() => {
		if(!is_loading && items.length < range && index > (range - items.length))
			index -= (range - items.length);
	});

	let reverse_sign = $derived(reversed ? -1 : 1);

	let is_loading = $derived(items.length > 0 && Object.keys(items[items.length - 1]).length === 0);

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
	const remember_scroll_pos = () => {
		scrollTop = list_div.scrollTop;
		anchor_id = items[Math.floor(items.length / 2)].id;
		anchor_top_before = get_anchor(anchor_id).offsetTop;
		list_scroll_top_before = list_div.scrollTop;
	}

	const on_scroll = (e) => {
		if(is_loading)
			return;

		remember_scroll_pos();

		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		if(reverse_sign * next_scroll_top >= max_scroll || reverse_sign * next_scroll_top < 0){
			let dir = Math.sign(e.deltaY);

			if(dir !== 0 && index + reverse_sign * dir >= 0){
				index += reverse_sign * dir * advance;
				if(index < 0) index = 0;
			}
		}
		list_div.scrollTop = next_scroll_top;
		scrollTop = list_div.scrollTop;
	};

	// show "Go to latest" button if scrolled more than half-page above last element
	let show_goto_latest = $derived(typeof to_latest_text !== "undefined" &&
		(index > 0 || 
		(typeof list_div !== "undefined" && (reverse_sign * scrollTop > (list_div.scrollHeight - list_div.clientHeight) * 0.5))
		));
 
	const goto_latest = () => {
		keep_scroll_pos = false;
		index = 0;
		list_div.scrollTop = 0;
		scrollTop = list_div.scrollTop;
	};
</script>

<div class="paginated_list">
	<div class="paginated_list" style={reversed ? "flex-direction: column-reverse" : ""} onwheel={on_scroll} bind:this={list_div}>
		{#each items as item, i}
			{#if Object.keys(item).length > 0}
				{@render render_item(i, item)}
			{/if}
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
