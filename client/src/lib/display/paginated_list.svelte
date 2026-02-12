<script>
	import {asset} from '$app/paths';
	import {tick} from 'svelte';
	import {RangeCache} from '$lib/cache/range.svelte.js';

	let {scrollTop = $bindable(0),
		range = 30, advance = 10, reversed = false,
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

	export function reset(){
		init_items = load_items(reversed ? RangeCache.max_id : 0, range, !reversed);
		items = {loaded: false, data: []};

		list_div.scrollTop = 0;
	}

	$effect(() => {
		if(items.loaded && !items.is_full){
			console.log("finalizing items");
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

	const on_scroll = (e) => {
		if(!items.loaded)
			return;

		let max_scroll = list_div.scrollHeight - list_div.clientHeight;
		let next_scroll_top = list_div.scrollTop + e.deltaY * 0.3;

		if(reverse_sign * next_scroll_top >= max_scroll || reverse_sign * next_scroll_top < 0){
			let dir = Math.sign(e.deltaY) * reverse_sign;

			if(dir > 0 && items.is_full)
				items = load_items(items.data[advance - 1].id + reverse_sign, range, !reversed);
			else if(dir < 0 && can_scroll_before)
				items = load_items(items.data[items.data.length - advance].id - reverse_sign, range, reversed);
		} else {
			list_div.scrollTop = next_scroll_top;
			scrollTop = list_div.scrollTop;
		}
	};

	// show "Go to latest" button if scrolled more than half-page above last element
	let show_goto_latest = $derived(typeof to_latest_text !== "undefined" &&
		(can_scroll_before ||
		(typeof list_div !== "undefined" && (reverse_sign * scrollTop > (list_div.scrollHeight - list_div.clientHeight) * 0.5))
		));
</script>

<div class="paginated_list"
	style="max-height:var(--max-height, 100%);"
>
	<div class="paginated_list" style={reversed ? "flex-direction: column-reverse" : ""} onwheel={on_scroll} bind:this={list_div}>
		{#each items.data as item, i}
			{#if item}
				<div id={"paginated_list_item_" + item.id}>
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
