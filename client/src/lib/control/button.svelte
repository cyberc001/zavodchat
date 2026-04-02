<script>
	const {text, icon,
		disabled = false,
		onclick} = $props();
</script>

<button class="settings_control settings_button" 
style="width: var(--width, auto); margin-bottom: var(--margin-bottom, 12px); margin-top: var(--margin-top, 0px); margin-left: var(--margin-left, 0px); padding-bottom: var(--padding-bottom, revert)"
onclick={onclick} {disabled}
>
	{text}
	{#if icon}
		<img src={icon} class="filter_icon_main" style="height: var(--icon_height, 32px)"/>
	{/if}
</button>

<style>
@import "style.css";
</style>
