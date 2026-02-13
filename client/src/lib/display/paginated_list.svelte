<script>
	import {asset} from '$app/paths';
	import {tick} from 'svelte';
	import {RangeCache} from '$lib/cache/range.svelte.js';

	let {range = 40, advance = 10, reversed = false,
		render_item, load_items, augment_item = () => {},
		loading_text = "Loading...", to_latest_text = "To latest"
	} = $props();

	let init_items = $state(load_items(reversed ? RangeCache.max_id : 0, range, !reversed));
	let items = $state({loaded: false, data: []});
	// Initialize items with init_items (dont duplicate the request)
	$effect(() => {
		if(typeof items.start_id === "undefined" && init_items.loaded)
			items = init_items.clone();
	});

	$effect(() => {
		console.log("INIT ITEMS CHANGES", $state.snapshot(init_items.data));
	});

	let item_divs = $state([]), item_divs_dict = $state({});

	export function reset(){
		init_items = load_items(reversed ? RangeCache.max_id : 0, range, !reversed);
		items = {loaded: false, data: []};

		list_div.scrollTop = 0;
	}

	$effect(() => {
		if(items.loaded && !items.is_full && !items.final){
			console.log("finalizing items", items, $state.snapshot(items.data));
			items = load_items(items.asc ? RangeCache.max_id : 0, range, !items.asc);
			items.final = true;
		}
	});

	export function getItemCount(){
		return items.data.length;
	}
	export function getItem(idx){
		return items.data[idx];
	}
	export function findItem(filter){
		return items.data.find(filter);
	}

	// A method for loading additional information into items. Its useful, because:
	// 1. You cannot use cached REST methods in snippets that PaginatedList uses to render items, since it makes PaginatedList mutate static state (i.e. cache) that Main owns
	// 2. Same goes for embedding data directly in REST cached requests using other REST cached requests
	$effect(() => {
		for(let item of items.data)
			if(item)
				augment_item(item);
	});

	let reverse_sign = $derived(reversed ? -1 : 1);
	let can_scroll_before = $derived(init_items.data.length > 0 && items.data.length > 0 && reverse_sign * (items.data[0].id - init_items.data[0].id) > 0);

	let list_div = $state();
	let content_div = $state();

	// Scroll calculations
	let items_offset_top = $state(0);
	let items_max_height = $state(0);

	let div_items = $state({});
	// temporary dictionary used to remember global offsets of previously rendered items
	let id_to_offset = {};

	const remember_offsets = () => {
		if(!items.loaded)
			return;
		id_to_offset = {};
		for(const i in div_items)
			if(items.data[i] && div_items[i])
				id_to_offset[items.data[i].id] = {
									top: content_div.offsetTop + div_items[i].offsetTop,
									bottom: content_div.offsetTop + get_el_bottom(div_items[i])
								};
		console.trace();
		console.log("remember offsets", id_to_offset, $state.snapshot(div_items), $state.snapshot(items.data));
	};

	$effect(() => {
		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1])
			return;
		let items_height = get_el_bottom(div_items[items.data.length - 1]);
		if(items_height > items_max_height)
			items_max_height = items_height;
		console.log("items max height", items_max_height);
	});
	$effect(() => {
		items.data;

		console.log("new items", $state.snapshot(items.data));

		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1] || items.adjusted)
			return;

		if(typeof id_to_offset[items.data[0].id] !== "undefined"){
			items_offset_top = id_to_offset[items.data[0].id].top;
			console.log("recall top offset\n", id_to_offset, "\n", $state.snapshot(items.data));
		} else if(typeof id_to_offset[items.data[items.data.length - 1].id] !== "undefined"){
			items_offset_top = id_to_offset[items.data[items.data.length - 1].id].bottom - items_max_height;
			console.log("recall bottom offset\n", id_to_offset, "\n", $state.snapshot(items.data));
		} else {
			items_offset_top = 0;
			if(reversed)
				list_div.scrollTop = items_max_height - list_div.clientHeight;
			console.log("recall initial offset", list_div.scrollTop, items_max_height -= list_div.clientHeight);
		}

		if(items.loaded)
			items.adjusted = true;
	});

	$effect(() => {
		console.log("items_offset_top", items_offset_top);

		// DO NOT CHANGE THE ORDER OF THESE TWO ASSIGMENTS
		if(items_offset_top < 0){
			list_div.scrollTop = -items_offset_top;
			items_offset_top = 0;
		}
	});

	const get_el_bottom = (el) => el.offsetTop + el.clientHeight;

	const on_scroll = (e) => {
		if(!items.loaded || items.data.length === 0)
			return;

		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		const max_scroll = content_div.offsetTop + get_el_bottom(div_items[items.data.length - 1]);
		console.log("real scroll", next_scroll_top, list_div.clientHeight, "max scroll", max_scroll, "off", content_div.offsetTop);

		if(next_scroll_top + list_div.clientHeight >= max_scroll || next_scroll_top < content_div.offsetTop){
			let dir = Math.sign(e.deltaY) * reverse_sign;

			remember_offsets();

			console.log("checking to load more items", dir, items.is_full, can_scroll_before, $state.snapshot(items.data), $state.snapshot(init_items.data));
			if(dir > 0 && items.is_full){
				console.log("loading more items", $state.snapshot(items.data), items.data[reversed ? items.data.length - advance : advance - 1].id + reverse_sign, !reversed);
				items = load_items(items.data[reversed ? items.data.length - advance : advance - 1].id  + reverse_sign, range, !reversed);
			} else if(dir < 0 && can_scroll_before){
				console.log("loading more items", $state.snapshot(items.data), items.data[reversed ? advance - 1 : items.data.length - advance].id - reverse_sign, reversed);
				items = load_items(items.data[reversed ? advance - 1 : items.data.length - advance].id - reverse_sign, range, reversed);
			}

			// Scroll to the max
			if(next_scroll_top + list_div.clientHeight >= max_scroll)
				list_div.scrollTop = max_scroll - list_div.clientHeight;
			else {
				console.log("setting scrollTop");
				list_div.scrollTop = content_div.offsetTop;
			}
		} else
			list_div.scrollTop = next_scroll_top;
	};

	// show "Go to latest" button if scrolled more than half-page above last element
	let show_goto_latest = $derived(false);
	/*let show_goto_latest = $derived(typeof to_latest_text !== "undefined" &&
		(can_scroll_before ||
		(typeof list_div !== "undefined" && (reverse_sign * scrollTop > (list_div.scrollHeight - list_div.clientHeight) * 0.5))
		));*/
</script>

<div class="paginated_list"
	style="max-height:var(--max-height, 100%);"
>
	<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
		<div style={`position: absolute; top: ${items_offset_top}px; min-height: ${items_max_height}px`}
			bind:this={content_div}
		>
		{#each items.data as item, i}
			{#if item}
				<div bind:this={div_items[i]}
				>
					{item.id}
					{@render render_item(i, item)}
				</div>
			{/if}
		{/each}
		</div>
	</div>
	{#if !items.loaded}
		<div class="item paginated_list_overlay">
			<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 20px; margin-right: 8px"/>
			<span class="paginated_list_overlay_text">{loading_text}</span>
		</div>
	{/if}
	{#if show_goto_latest}
		<button class="item paginated_list_overlay paginated_list_overlay_tolatest" onclick={() => reset()}>
			<span class="paginated_list_overlay_text" style="text-decoration: underline">{to_latest_text}</span>
		</button>
	{/if}
</div>

<style>
.paginated_list {
	height: 100%;
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
