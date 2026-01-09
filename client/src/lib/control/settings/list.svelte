<script>

let {items = $bindable([]), selected_idx = $bindable(-1),
	render_item} = $props();

const onClick = (i) => {
	selected_idx = i;
};

</script>

{#each items as item, i}
	<div class={"hoverable" + (i === selected_idx ? " selected" : "")}
		onclick={() => onClick(i)}
	>
		{@render render_item(i, item)}
	</div>
{/each}
