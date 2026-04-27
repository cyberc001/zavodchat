<script>
	import {asset} from '$app/paths';
	import {tick, untrack} from 'svelte';
	import {RangeCache} from '$lib/cache/range.svelte.js';

	let {range = 40, advance = 10, reversed = false,
		render_item, prepend_items = [], load_items,
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
			if(!last_item)
				return;
			console.log("NEW ITEM CHECK", get_abs_scroll(), last_item.clientHeight);
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
	}

	$effect(() => {
		if(items.loaded && !items.is_full && !items.final && init_items.loaded){
			console.log("finalizing items", items, $state.snapshot(items.data), start_id);

			items.destroy();
			if(!init_items.is_full)
				items = init_items.clone();
			else
				items = _load_items(items.asc ? RangeCache.max_id : 0, range, !items.asc);
			items.final = true;
		}
	});

	export function isLoaded(){
		return items ? items.loaded : false;
	}
	export function getItemCount(){
		return items.data.length;
	}
	export function getItem(idx){
		return items.data[idx];
	}
	export function findItem(filter){
		return items.data.find(filter);
	}

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

		if(!div_items[i])
			return;

		console.log("SETTING ANCHOR", i, $state.snapshot(items.data), $state.snapshot(div_items));

		anchor = {
			id: items.tree.key(items.data[i]),
			top: div_items[i].offsetTop - list_div.scrollTop
		};
	};

	let list_div_scroll_height = $state(0);

	let list_resize_obs;
	let list_mut_obs;
	const list_mut_cb = (elements) => {
		// Observers might be active when this component is off-screen
		if(!list_div)
			return;
		list_div_scroll_height = list_div.scrollHeight;

		if(typeof(anchor.top) === "undefined")
			list_div_scroll_top = list_div.scrollTop = reversed ? 2147483648 : 0;
		else {
			const anchor_el = div_items[items.data.findIndex((x) => items.tree.key(x) === anchor.id)];
			console.log("ANCHOR", anchor, "ITEMS", $state.snapshot(items.data), "\n", anchor_el, "\n", list_div.scrollTop, anchor_el?.offsetTop);
			if(anchor_el)
				list_div.scrollTop += (anchor_el.offsetTop - list_div.scrollTop) - anchor.top;	
		}
	};
	$effect(() => {
		if(!list_div)
			return;
		list_resize_obs = new ResizeObserver(list_mut_cb);
		list_resize_obs.observe(list_div);
		for(const item of Object.values(div_items))
			if(item)
				list_resize_obs.observe(item);

		list_mut_obs = new MutationObserver(list_mut_cb);
		list_mut_obs.observe(list_div, {childList: true, subtree: true, attributes: true});
	});

	$effect(() => {
		items;
		list_div_scroll_height = list_div.scrollHeight;
	});

	const on_scroll = (e) => {
		list_div_scroll_top = list_div.scrollTop;
		if(!items.loaded || items.data.length === 0)
			return;

		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		const max_scroll = list_div.scrollHeight - list_div.clientHeight;
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

				items = _load_items(items.tree.key(items.data[reversed ? items.data.length - advance : advance - 1]) + reverse_sign, range, !reversed);
			} else if(dir < 0 && can_scroll_before){
				destroy_old_items = true;

				console.log("loading more items", $state.snapshot(items.data), items.tree.key(items.data[reversed ? advance - 1 : items.data.length - advance]) - reverse_sign, reversed);
				items = _load_items(items.tree.key(items.data[reversed ? advance - 1 : items.data.length - advance]) - reverse_sign, range, reversed);
			}
			if(destroy_old_items)
				old_items.destroy();
		} else
			list_div.scrollTop = next_scroll_top;

		if(typeof anchor.id !== "undefined")
			anchor.top += (list_div_scroll_top - list_div.scrollTop);
		else if(div_items[0])
			set_anchor(0);

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
		{#each prepend_items as item, i}
			{@render render_item(i, item)}
		{/each}
		{#each items.data as item, i}
			{#if item}
				<div bind:this={div_items[i]}>
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

	overflow-x: hidden;
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
