<script>
	const {text, onclick, disabled = false} = $props();
</script>

<button class="settings_control settings_button" onclick={onclick} {disabled}>{text}</button>

<style>
@import "style.css";
.settings_button {
	margin-bottom: 12px;
}
</style>
