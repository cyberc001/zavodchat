<script>
	import {asset} from '$app/paths';
	import {tick} from 'svelte';
	import {RangeCache} from '$lib/cache/range.svelte.js';

	let {range = 40, advance = 10, reversed = false,
		render_item, load_items, augment_item = () => {},
		loading_text = "Loading...", to_latest_text = "To latest",
		auto_height = false
	} = $props();

	let init_items = $state(load_items(reversed ? RangeCache.max_id : 0, range, !reversed));
	let items = $state({loaded: false, data: []});
	// Initialize items with init_items (dont duplicate the request)
	$effect(() => {
		if(typeof items.start_id === "undefined" && init_items.loaded && typeof items.start_id === "undefined"){
			console.log("SETTING ITEMS TO INIT_ITEMS");
			items = init_items.clone();
		}
	});

	let item_divs = $state([]), item_divs_dict = $state({});
	let items_max_height = $state(0);
	// temporary dictionary used to remember global offsets of previously rendered items
	let id_to_offset = {};
	let list_div_scroll_top = $state(0);

	let start_id;
	export function reset(_start_id){
		start_id = _start_id;
		console.log("RESETTING", start_id);
		init_items = load_items(reversed ? RangeCache.max_id : 0, range, !reversed);
		if(typeof start_id === "undefined")
			items = {loaded: false, data: []};
		else
			items = load_items(start_id, range, !reversed);
	
		list_div_scroll_top = list_div.scrollTop = 0;
		items_max_height = 0;
		id_to_offset = {};
	}
	
	$effect(() => {
		if(items.loaded && !items.is_full && !items.final && init_items.loaded){
			console.log("finalizing items", items, $state.snapshot(items.data), start_id);
			if(!init_items.is_full)
				items = init_items.clone();
			else
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

	let div_items = $state({});

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
		console.log("remember offsets", id_to_offset, $state.snapshot(div_items), $state.snapshot(items.data));
	};

	let list_div_scroll_height = $state(0);
	$effect(() => {
		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1])
			return;
		let items_height = get_el_bottom(div_items[items.data.length - 1]);
		if(items_height > items_max_height)
			items_max_height = items_height;
		list_div_scroll_height = list_div.scrollHeight;
	});

	$effect(() => {
		list_div_scroll_top = list_div.scrollTop;

		console.log("new items", items, $state.snapshot(items.data), id_to_offset);
		if(items.last_action === "updated" && div_items[0].clientHeight > 0)
			remember_offsets();
		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1] || items.adjusted || div_items[0].clientHeight === 0)
			return;

		// If list was scrolled to the last item, dont remember offsets (it might be a new item)
		const scroll_off = list_div.scrollTop + list_div.clientHeight;
		const last_off = id_to_offset[items.data[items.data.length - 2]?.id];

		console.log("check", items.last_action, can_scroll_before, last_off, scroll_off, last_off?.top, last_off?.bottom);
		if(items.last_action === "removed" || (items.last_action === "inserted" && reversed && !can_scroll_before && last_off && scroll_off >= last_off.top && scroll_off <= last_off.bottom)){
			console.log("reset offsets");
			id_to_offset = {};
		}

		if(typeof id_to_offset[items.data[0].id] !== "undefined"){
			items_offset_top = id_to_offset[items.data[0].id].top;
			console.log("recall top offset\n", id_to_offset, "\n", $state.snapshot(items.data), /*"\n", items_offset_top*/);
		} else if(typeof id_to_offset[items.data[items.data.length - 1].id] !== "undefined"){
			let items_height = get_el_bottom(div_items[items.data.length - 1]);
			if(items_height === 0)
				return;
			items_offset_top = id_to_offset[items.data[items.data.length - 1].id].bottom - items_height;
			console.log("recall bottom offset\n", id_to_offset, "\n", $state.snapshot(items.data), "\n", /*items_offset_top, "\n",*/ items.data[items.data.length - 1].id);
		} else {
			items_offset_top = 0;
			if(reversed)
				list_div.scrollTop = items_max_height - list_div.clientHeight;
			console.log("recall initial offset", list_div.clientHeight, list_div.scrollTop, items_max_height - list_div.clientHeight);
		}

		if(typeof start_id !== "undefined" && items.loaded){
			let start_div = div_items[items.data.findIndex((x) => x.id === start_id)];
			console.log("SETTING TO START_ID", start_id, start_div, list_div.scrollTop, start_div.getBoundingClientRect().top);
			start_div.scrollIntoView(true);
			console.log("AFTER", list_div.scrollTop);
			start_id = undefined;
		}

		if(items.loaded)
			items.adjusted = true;

		remember_offsets();
		list_div_scroll_top = list_div.scrollTop;
	});

	$effect(() => {
		console.log("items_offset_top", items_offset_top);

		// DO NOT CHANGE THE ORDER OF THESE TWO ASSIGMENTS
		if(items_offset_top < 0){
			list_div_scroll_top = list_div.scrollTop = -items_offset_top;
			items_offset_top = 0;
		}
	});

	const get_el_bottom = (el) => el.offsetTop + el.clientHeight;

	const on_scroll = (e) => {
		list_div_scroll_top = list_div.scrollTop;
		if(!items.loaded || items.data.length === 0)
			return;

		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		const max_scroll = content_div.offsetTop + get_el_bottom(div_items[items.data.length - 1]);
		if(next_scroll_top + list_div.clientHeight >= max_scroll || next_scroll_top < content_div.offsetTop){
			let dir = Math.sign(e.deltaY) * reverse_sign;

			remember_offsets();

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
		list_div_scroll_top = list_div.scrollTop;
	};

	// For some unknown reason $derived() with the same condition does not work. Rendering related?
	let show_goto_latest = $state(false);
	$effect(() => {
		show_goto_latest = typeof to_latest_text !== "undefined" && (can_scroll_before || (list_div && (reversed ? list_div.scrollHeight - list_div.clientHeight - list_div_scroll_top : list_div_scroll_top) > items_max_height * 0.3));
	});
</script>

<div class="paginated_list"
	style={(auto_height ? `height: ${list_div_scroll_height}px;` : "")
		+ "max-height: var(--max-height, 100%); width: var(--width, auto)"}
>
	<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
		<div style={`position: absolute; top: ${items_offset_top}px; min-height: ${items_max_height}px`}
			bind:this={content_div}
		>
		{#each items.data as item, i}
			{#if item}
				<div bind:this={div_items[i]}
				>
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
