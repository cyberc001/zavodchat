<script>
	let {label_text, error = "",
		is_password = false, value = $bindable("")} = $props();
</script>

<div class="settings_textbox_frame">
	<p class="settings_control_label">{label_text}</p>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" type={is_password ? "password" : "input"}
	class="settings_control" style="width:var(--width, 512px)"
	bind:value={value}/>
	<p class="settings_control_label" style="color: var(--clr_text_error)">{error}</p>
</div>

<style>
@import "style.css";
.settings_textbox_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
</style>
