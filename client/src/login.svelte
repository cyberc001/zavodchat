<script>
	import SettingsTextbox from './common/settings_textbox.svelte';
</script>

<div class="login_frame">
<p style="margin: 0; margin-bottom: 20px;">Log in</p>
<SettingsTextbox label_text="username"/>
<p style="margin: 0; margin-bottom: 6px;"/>
<SettingsTextbox label_text="password" is_password=true/>
</div>

<style>
	@import "login.css";
</style>
