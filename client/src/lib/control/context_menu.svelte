<script>
	import FocusManager from '$lib/focus_manager.svelte';

	const { anchor, off = [0, 0],
		hide_ctx_menu,
		items } = $props();

	let self = $state();
</script>


<FocusManager element={self}
	onblur={() => {
		hide_ctx_menu();
	}}
/>


<div class="item context_menu_panel"
	style="position-anchor: {getComputedStyle(anchor).getPropertyValue("anchor-name")}; left: calc(anchor(left, 10000px) + {off[0]}px); top: calc(anchor(top, -10000px) + {off[1]}px)"
	role="list"
	bind:this={self}
>
	{#each items as item, i}
		<div class="item context_menu_item hoverable">
			{@render item(hide_ctx_menu, i)}
		</div>
	{/each}
</div>

<style>
.context_menu_panel {
	position: fixed;
	position-visibility: no-overflow;
	width: fit-content;
	z-index: 20;
}
.context_menu_item {
	width: 100%;
	display: flex;
	align-items: center;

	border: none;

	font-size: 18px;
	color: var(--clr_text);
}
.context_menu_item + .context_menu_item {
	border-width: 2px;
	border-color: var(--clr_bg_selected);
	border-style: solid none none none;
}
</style>
