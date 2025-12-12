<script>
	const {text, onclick, disabled = false} = $props();
</script>

<button class="settings_control" onclick={onclick} {disabled}>{text}</button>

<style>
@import "style.css";
</style>
