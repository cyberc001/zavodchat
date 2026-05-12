<script>
	let {icon, text,
		onclick, hide_ctx_menu} = $props();
</script>

<button class="transparent_button context_menu_action" onclick={() => {
		onclick();
		hide_ctx_menu();
}}>
	{#if typeof icon !== "undefined"}
		<img src={icon} class="context_menu_icon filter_icon_main" alt={text}/>
	{/if}
	<div style="margin-left: 2px">{text}</div>
</button>

<style>
.context_menu_action {
	display: flex;
	width: 100%;
	align-items: center;
	padding: 2px 4px 2px 2px;

	font-size: 16px;
	color: var(--clr_text);
}
.context_menu_action:hover {
	background: none;
}

.context_menu_icon {
	height: 16px;
}
</style>
