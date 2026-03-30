<script>
	import {asset} from '$app/paths';
	import {tick, untrack} from 'svelte';
	import {RangeCache} from '$lib/cache/range.svelte.js';

	let {range = 40, advance = 10, reversed = false,
		render_item, load_items, augment_item = () => {},
		loading_text = "Loading...", to_latest_text = "To latest",
		auto_height = false
	} = $props();

	const _load_items = (start_id, count, asc) => {
		const new_items = load_items(start_id, count, asc);
		new_items.onupdate = (obs, id) => {
			set_anchor(items.data.findIndex((x) => obs.tree.key(x) === id));
		};
		new_items.oninsert = (obs, id) => {
			if(!can_scroll_before || div_items.length === 0)
				return;

			const last_item = div_items[reversed ? items.data.length - 1 : 0];
			if(get_abs_scroll() > last_item.clientHeight)
				set_anchor(reversed ? items.data.length - 1 : 0);
			else
				anchor = {};
		};
		new_items.onremove = (obs, id) => {
			if(obs === init_items)
				return;
				
			let i = 0;
			for(; i < items.data.length; ++i)
				if(obs.tree.key(items.data[i]) === id)
					break;
			if(i === 0)
				i = 1;
			else
				--i;

			console.log("SETTING ANCHOR", i, "\n", $state.snapshot(items.data), obs, $state.snapshot(obs.data));

			set_anchor(i);
		};
		return new_items;
	};

	let init_items = $state(_load_items(reversed ? RangeCache.max_id : 0, range, !reversed));
	let items = $state({loaded: false, data: []});
	// Initialize items with init_items (dont duplicate the request)
	$effect(() => {
		if(typeof items.start_id === "undefined" && init_items.loaded && typeof items.start_id === "undefined"){
			items = init_items.clone();
		}
	});

	let item_divs = $state([]), item_divs_dict = $state({});
	let anchor = {};
	let list_div_scroll_top = $state(0);

	let start_id;
	export function reset(_start_id){
		start_id = _start_id;
		init_items = _load_items(reversed ? RangeCache.max_id : 0, range, !reversed);
		if(typeof start_id === "undefined")
			items = {loaded: false, data: []};
		else
			items = _load_items(start_id, range, !reversed);
	
		anchor = {};
		scroll_initialized = false;
	}

	$effect(() => {
		if(items.loaded && !items.is_full && !items.final && init_items.loaded){
			console.log("finalizing items", items, $state.snapshot(items.data), start_id);

			last_list_div_height = list_div.scrollHeight;

			items.destroy();
			if(!init_items.is_full)
				items = init_items.clone();
			else
				items = _load_items(items.asc ? RangeCache.max_id : 0, range, !items.asc);
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
	let can_scroll_before = $derived.by(() => {
		return (init_items.data.length > 0 && items.data.length > 0 && reverse_sign * (items.tree.key(items.data[0]) - init_items.tree.key(init_items.data[0])) > 0);
	});
	//let can_scroll_before = $derived(init_items.data.length > 0 && items.data.length > 0 && reverse_sign * (items.tree.key(items.data[0]) - init_items.tree.key(init_items.data[0])) > 0);

	let list_div = $state();

	// Scroll calculations
	let div_items = $state({});

	export function set_anchor(i){
		if(items.data.length === 0)
			return;

		if(typeof i === "undefined")
			i = 0;

		// Paginated list can be hidden, but updates can still happen (like when changing user profile)
		if(div_items[0] === null)
			return;

		if(i >= items.data.length)
			i = items.data.length - 1;

		anchor = {
			id: items.tree.key(items.data[i]),
			top: div_items[i].offsetTop - list_div.scrollTop
		};
	};

	let list_div_scroll_height = $state(0);
	$effect(() => {
		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1])
			return;
		list_div_scroll_height = list_div.scrollHeight;
	});

	let scroll_initialized = false;
	$effect(() => {
		//$inspect.trace();
		list_div_scroll_top = list_div.scrollTop;

		console.log("new items", list_div.scrollTop, "\n", items, $state.snapshot(items.data));
		if(items.data.length === 0 || !div_items[0] || !div_items[items.data.length - 1] || div_items[0].clientHeight === 0)
			return;

		if(items.loaded && !scroll_initialized){
			scroll_initialized = true;
			list_div_scroll_top = list_div.scrollTop = reversed ? 2147483648 : 0;
		}

		if(typeof anchor.id !== "undefined"){
			const anchor_el = div_items[items.data.findIndex((x) => items.tree.key(x) === anchor.id)];
			console.log("ANCHOR", anchor, "ITEMS", $state.snapshot(items.data), "\n", anchor_el, "\n", list_div.scrollTop, anchor_el?.offsetTop);
			if(anchor_el)
				list_div.scrollTop += (anchor_el.offsetTop - list_div.scrollTop) - anchor.top;
			console.log("SCROLL TOP AFTER ANCHORING", list_div.scrollTop);
		}
	
		if(typeof start_id !== "undefined" && items.loaded){
			let start_div = div_items[items.data.findIndex((x) => items.tree.key(x) === start_id)];
			start_div.scrollIntoView(true);
			start_id = undefined;
		}

		list_div_scroll_top = list_div.scrollTop;
	});

	// Scroll back to bottom/top if user hasnt scrolled past the last element and the element changed in size
	// (ex. an image attachment got loaded)
	// It is assumed that div_items only grow in size and not shrink (therefore only previous list_div.scrollHeight is kept track of, and the same is false for div_items)
	let list_resize_obs;
	let last_list_div_height = -1;
	$effect(() => {
		if(!list_div)
			return;
		list_resize_obs = new ResizeObserver((elements) => {
			if(!list_div || items.data.length === 0 || div_items.length === 0)
				return;

			const last_item = div_items[reversed ? items.data.length - 1 : 0];
			console.log("SCROLL CONDITION", list_div.scrollTop + last_item.clientHeight, last_list_div_height - list_div.clientHeight, "\n", list_div.scrollTop, last_item.clientHeight, last_list_div_height, list_div.clientHeight, "\n", $state.snapshot(items.data));
			if(reversed ? list_div.scrollTop + last_item.clientHeight >= last_list_div_height - list_div.clientHeight
					: list_div.scrollTop <= last_item.clientHeight){
				console.log("SCROLLING TO BOTTOM");
				list_div_scroll_top = list_div.scrollTop = reversed ? 2147483648 : 0;
			}
			last_list_div_height = list_div.scrollHeight;
		});
		list_resize_obs.observe(list_div);
		for(const item of Object.values(div_items))
			if(item)
				list_resize_obs.observe(item);
	});

	const on_scroll = (e) => {
		list_div_scroll_top = list_div.scrollTop;
		if(!items.loaded || items.data.length === 0)
			return;

		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		const max_scroll = list_div.scrollHeight - list_div.clientHeight;
		console.log("scroll", list_div.scrollTop, next_scroll_top, max_scroll);
		if(next_scroll_top >= max_scroll || next_scroll_top < 0){
			let dir = Math.sign(e.deltaY) * reverse_sign;

			if(next_scroll_top >= max_scroll)
				list_div.scrollTop = max_scroll;
			else
				list_div.scrollTop = 0;

			set_anchor(e.deltaY > 0 ? items.data.length - 1 : 0);

			const old_items = items;
			let destroy_old_items = false;
			if(dir > 0 && items.is_full){
				destroy_old_items = true;
				console.log("loading more items", $state.snapshot(items.data), items.tree.key(items.data[reversed ? items.data.length - advance : advance - 1]) + reverse_sign, !reversed);

				last_list_div_height = list_div.scrollHeight;

				items = _load_items(items.tree.key(items.data[reversed ? items.data.length - advance : advance - 1]) + reverse_sign, range, !reversed);
			} else if(dir < 0 && can_scroll_before){
				destroy_old_items = true;

				last_list_div_height = list_div.scrollHeight;

				console.log("loading more items", $state.snapshot(items.data), items.tree.key(items.data[reversed ? advance - 1 : items.data.length - advance]) - reverse_sign, reversed);
				items = _load_items(items.tree.key(items.data[reversed ? advance - 1 : items.data.length - advance]) - reverse_sign, range, reversed);
			}
			if(destroy_old_items)
				old_items.destroy();
		} else
			list_div.scrollTop = next_scroll_top;
		list_div_scroll_top = list_div.scrollTop;
	};

	// For some unknown reason $derived() with the same condition does not work. Rendering related?
	let show_goto_latest = $state(false);
	const get_abs_scroll = () => {
		if(!list_div)
			return 0;
		return reversed ? list_div.scrollHeight - list_div.clientHeight - list_div_scroll_top : list_div_scroll_top;
	}
	$effect(() => {
		show_goto_latest = typeof to_latest_text !== "undefined" && (can_scroll_before || get_abs_scroll() > list_div.scrollHeight * 0.3);
	});
</script>

<div class="paginated_list"
	style={(auto_height ? `height: ${list_div_scroll_height}px;` : "")
		+ "max-height: var(--max-height, 100%); width: var(--width, auto)"}
>
	<div class="paginated_list" onwheel={on_scroll} bind:this={list_div}>
		{#each items.data as item, i}
			{#if item}
				<div bind:this={div_items[i]}
				>
					{@render render_item(i, item)}
				</div>
			{/if}
		{/each}
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
