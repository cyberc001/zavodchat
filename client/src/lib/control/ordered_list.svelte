<script>

	let {items = $bindable([]), selected_idx = $bindable(-1),

		hoverable = true,

		check_select = (selected, selected_idx) => true,
		check_drag = (dragged, dragged_idx) => true,
		check_insert = (dragged, dragged_idx, hovered, hovered_idx) => true,

		// Return false to cancel order change
		on_drag = (dragged, dragged_idx, hovered, hovered_idx) => true,

		render_item
	} = $props();
	
	let div_items = $state([]);
	
	let dragged_idx = $state(-1);
	let hovered_idx = $state(-1);
	
	const onClick = (i) => {
		if(check_select(items[i], i))
			selected_idx = i;
	};
	
	const getDropIdx = (y) => {
		if(div_items.length === 0)
			return -1;
	
		const top_rect = div_items[0].getBoundingClientRect();
		if(y < top_rect.top + top_rect.height / 2)
			return 0;
	
		for(let i = 0; i < items.length - 1; ++i){
			const rect = div_items[i].getBoundingClientRect(),
			      rect_next = div_items[i + 1].getBoundingClientRect();
			if(y >= rect.top + rect.height / 2 && y < rect_next.top + rect_next.height / 2)
				return i + 1;
		}
	
		const bottom_rect = div_items[div_items.length - 1].getBoundingClientRect();
		if(y >= bottom_rect.top + bottom_rect.height / 2)
			return div_items.length;

		return -1;
	};
	const getIdx = (y) => {
		for(let i = 0; i < items.length; ++i){
			const rect = div_items[i].getBoundingClientRect();
			if(y >= rect.top && y <= rect.bottom)
				return i;
		}
		return -1;
	}
	
	const onDragStart = (e) => {
		dragged_idx = getIdx(e.clientY);
		if(!check_drag(items[dragged_idx], dragged_idx)){
			dragged_idx = -1;
			e.preventDefault();
		}
	};
	const onDragEnd = () => {
		if(hovered_idx > -1 && dragged_idx > -1 && hovered_idx !== dragged_idx &&
		on_drag(items[dragged_idx], dragged_idx, items[hovered_idx], hovered_idx)){
			// re-insert item into the array
			let item = items.splice(dragged_idx, 1)[0];
			if(dragged_idx < hovered_idx)
				--hovered_idx;
			items.splice(hovered_idx, 0, item);
			selected_idx = hovered_idx;
		}
		dragged_idx = hovered_idx = -1;
	};
	const onDrag = (e) => {
		let new_hovered_idx = getDropIdx(e.clientY);
		if(new_hovered_idx > -1 &&
		check_insert(items[dragged_idx], dragged_idx, items[new_hovered_idx], new_hovered_idx))
			hovered_idx = new_hovered_idx;
		else
			hovered_idx = -1;
	};

</script>

{#each items as item, i}
	<div draggable="true" class={"ordered_list_item" + (hoverable ? " hoverable " : "") + (i === selected_idx ? " selected" : "")}
		onclick={() => onClick(i)}
		ondragstart={onDragStart} ondragend={onDragEnd}
		ondragover={onDrag}
		bind:this={div_items[i]}

		style={(i === hovered_idx ? "border-style: solid none none none"
			: i === items.length - 1 && hovered_idx === items.length ? "border-style: none none solid none"
			: "")}
		role="menuitem"
	>
		{@render render_item(i, item)}
	</div>
{/each}

<style>
.ordered_list_item {
	border-color: var(--clr_border_focus);
	border-width: 3px;
}
</style>
