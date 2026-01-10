<script>
	const {text, onclick, disabled = false} = $props();
</script>

<button class="settings_control settings_button" 
style="width:var(--width, auto); margin-bottom:var(--margin-bottom, 12px); margin-top:var(--margin-top, 0px)"
onclick={onclick} {disabled}
>
	{text}
</button>

<style>
@import "style.css";
</style>
