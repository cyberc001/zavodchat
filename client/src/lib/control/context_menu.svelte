<script>
	const { pos, hide_ctx_menu,
		actions } = $props();

	let pointer_on_menu = false;
	let first_mouse_up = true;
	const onmouseup = () => {
		if(pointer_on_menu || first_mouse_up){
			first_mouse_up = false;
			return;
		}
		hide_ctx_menu();
	};

	const init_focus = (el, i) => {
		if(i == 0)
			el.focus();
	};
</script>

<svelte:window {onmouseup}/>
<div class="item context_menu_panel" style="left: {pos[0]}px; top: {pos[1]}px"
	role="list"
	onmouseenter={() => pointer_on_menu = true}
	onmouseleave={() => pointer_on_menu = false}>
{#each actions as a, i}
	<button class="item context_menu_item hoverable" use:init_focus={i}
		onclick={() => {
			a.func();
			hide_ctx_menu();
		}} onblur={(e) => {
			if(e.relatedTarget === null || !e.relatedTarget.classList.contains("context_menu_item"))
				hide_ctx_menu();
	}}>
		{#if typeof a.icon !== "undefined"}
			<img src={"/src/lib/assets/icons/actions/" + a.icon} class="filter_icon_main" alt={a.text}/>
		{/if}
		<div style="margin-left: 4px">{a.text}</div>
	</button>
{/each}
</div>

<style>
.context_menu_panel {
	position: absolute;
	width: fit-content;
}
.context_menu_item {
	width: 120%;
	display: flex;
	align-items: center;

	padding: 2px;

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
