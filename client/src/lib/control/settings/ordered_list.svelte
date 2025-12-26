<script>

let {items = $bindable([]), selected_idx = $bindable(-1),
	check_drag = (dragged, dragged_idx) => true,
	check_insert = (dragged, dragged_idx, hovered, hovered_idx) => true,
	render_item} = $props();

let div_items = $state([]);

let dragged_idx = $state(-1);
let hovered_idx = $state(-2);

const onClick = (i) => {
	selected_idx = i;
};

const getBeforeIdx = (y) => {
	if(div_items.length === 0)
		return -2;

	const top_rect = div_items[0].getBoundingClientRect();
	if(y < top_rect.top + top_rect.height / 2)
		return -1;

	const rect = div_items[items.length - 1].getBoundingClientRect();
	if(y >= rect.top && y <= rect.bottom)
		return div_items.length - 1;

	for(let i = 0; i < items.length - 1; ++i){
		const rect = div_items[i].getBoundingClientRect(),
		      rect_next = div_items[i + 1].getBoundingClientRect();
		if(y >= rect.top + rect.height / 2 && y < rect_next.top + rect.height / 2)
			return i;
	}

	const bottom_rect = div_items[div_items.length - 1].getBoundingClientRect();
	if(y >= bottom_rect.top && y <= bottom_rect.bottom)
		return div_items.length - 1;
	return -2;
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
	if(hovered_idx > -2 && dragged_idx > -1 && hovered_idx !== dragged_idx){
		let dragged_item = items[dragged_idx];
		items.splice(dragged_idx, 1);
		if(dragged_idx < hovered_idx)
			--hovered_idx;
		items.splice(hovered_idx + 1, 0, dragged_item);
		selected_idx = hovered_idx + 1;
	}
	dragged_idx = hovered_idx = -2;
};
const onDrag = (e) => {
	let new_hovered_idx = getBeforeIdx(e.clientY);
	if(check_insert(items[dragged_idx], dragged_idx, items[new_hovered_idx], new_hovered_idx))
		hovered_idx = new_hovered_idx;
	else
		hovered_idx = -2;
};

</script>

{#each items as item, i}
	<div draggable="true" class={"hoverable" + (i === selected_idx ? " selected" : "")}
		onclick={() => onClick(i)}
		ondragstart={onDragStart} ondragend={onDragEnd}
		ondragover={onDrag}
		bind:this={div_items[i]}
		style={(i === hovered_idx ? "border-style: none none solid none"
			: i === 0 && hovered_idx === -1 ? "border-style: solid none none none"
			: "")}
	>
		{@render render_item(item)}
	</div>
{/each}
