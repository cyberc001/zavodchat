<script>
	let {icon, text,
		onclick, hide_ctx_menu} = $props();
</script>

<button class="transparent_button context_menu_action" onclick={() => {
		onclick();
		hide_ctx_menu();
}}>
	{#if typeof icon !== "undefined"}
		<img src={icon} class="filter_icon_main" alt={text}/>
	{/if}
	<div style="margin-left: 4px">{text}</div>
</button>

<style>
.context_menu_action {
	display: flex;
	width: 100%;
	align-items: center;
	padding: 2px;

	font-size: 18px;
	color: var(--text_clr);
}
.context_menu_action:hover {
	background: none;
}
</style>
