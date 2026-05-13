<script>
	const {
		text, icon,
		disabled = false,
		onclick
	} = $props();
</script>

<button class="settings_control" 
	onclick={onclick} {disabled}
>
	{text}
	{#if icon}
		<img src={icon} class="filter_icon_main" style="height: var(--icon_height, 24px)"/>
	{/if}
</button>

<style>
@import "style.css";
</style>
