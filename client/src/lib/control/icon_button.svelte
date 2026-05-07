<script>
	const {icon,
		filter_class = "filter_icon_main",
		disabled = false,
		onclick} = $props();
</script>

<button class="settings_icon_button hoverable"
onclick={onclick} {disabled}
>
	<img src={icon} class={filter_class} style="height: var(--height, 24px)"/>
</button>

<style>
.settings_icon_button {
	border-style: none;
	padding: 0;
}
.settings_icon_button:not(.selected):not(:hover) {
	background: transparent;
}
.settings_icon_button:disabled > img {
	filter: brightness(0) saturate(100%) invert(47%) sepia(6%) saturate(243%) hue-rotate(212deg) brightness(98%) contrast(88%);
}
</style>
