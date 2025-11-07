<script>
	let {label_text, is_password = false, value = $bindable("")} = $props();
</script>

<div class="login_textbox_frame">
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false" type={is_password ? "password" : "input"} class="login_textbox" bind:value={value}/>
	<p class="login_textbox_label">{label_text}</p>
</div>

<style>
.login_textbox_frame {
	display: flex;
	flex-direction: column;
}
.login_textbox {
	font-size: 20px;
	color: #F6F5F5;

	background: #2A2E34;

	border-color: #767676;
	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}
.login_textbox:focus {
	outline: none;

	border-color: #C8C0C0;
	border-style: solid;
}
.login_textbox::selection {
	background: #644646;
}

.login_textbox_label {
	text-align: left;
	margin: 0;
	font-size: 16px;
}
</style>
