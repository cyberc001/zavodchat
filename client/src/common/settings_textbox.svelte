<script>
	const {label_text, is_password = false} = $props();
</script>

<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" type={is_password ? "password" : "input"} class="settings_textbox"/>
<p class="settings_label">{label_text}</p>

<style>
.settings_textbox {
	font-size: 20px;
	color: #F6F5F5;

	background: #2A2E34;

	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}
.settings_textbox:focus {
	outline: none;

	border-color: #C8C0C0;
	border-style: solid;
}
.settings_textbox::selection {
	background: #644646;
}

.settings_label {
	text-align: left;
	margin: 0;
	font-size: 16px;
}
</style>
