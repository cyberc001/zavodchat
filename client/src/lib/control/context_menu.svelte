<script>
	const { anchor, off = [0, 0],
		hide_ctx_menu,
		items } = $props();

	let pointer_on_menu = false;
	let first_mouse_up = true;
	const onmouseup = () => {
		if(pointer_on_menu || first_mouse_up){
			first_mouse_up = false;
			return;
		}
		hide_ctx_menu();
	};
</script>

<svelte:window {onmouseup}/>
<div class="item context_menu_panel" style="position-anchor: {getComputedStyle(anchor).getPropertyValue("anchor-name")};
						left: calc(anchor(left, 10000px) + {off[0]}px); top: calc(anchor(top, -10000px) + {off[1]}px)"
	role="list"
	onmouseenter={() => pointer_on_menu = true}
	onmouseleave={() => pointer_on_menu = false}
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
