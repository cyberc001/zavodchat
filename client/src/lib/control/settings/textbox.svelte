<script>
	let {label_text, error = "",
		is_password = false, value = $bindable("")} = $props();
</script>

<div class="settings_textbox_frame">
	<p class="settings_textbox_label">{label_text}</p>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" type={is_password ? "password" : "input"} class="settings_textbox" bind:value={value}/>
	<p class="settings_textbox_label" style="color: var(--clr_text_error)">{error}</p>
</div>

<style>
.settings_textbox_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
.settings_textbox {
	font-size: 20px;
	color: var(--clr_text);

	width: var(--width, 512px);

	background: var(--clr_bg_item);

	border-color: var(--clr_border_item);
	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}
.settings_textbox:focus {
	outline: none;

	border-color: var(--clr_border_focus);
	border-style: solid;
}

.settings_textbox_label {
	text-align: left;
	margin: 0;
	margin-bottom: 4px;
	font-size: 16px;
}
</style>
