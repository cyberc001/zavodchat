<script>
	import TextBox from '$lib/control/login/textbox.svelte';
	import Button from '$lib/control/login/button.svelte';
	import StatusBox from '$lib/control/login/statusbox.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';

	let { setPage } = $props();

	let username = $state(""), displayname = $state(""),
		password = $state(""), password_repeat = $state("");

	let status_text = $state("");
	let is_error = $state(true);
	
	let register = function(){
		if(password != password_repeat){
			is_error = true;
			status_text = "Repeated password mismatch";
			return;
		}

		Auth.register(username, displayname, password,
				() => {
					is_error = false;
					status_text = "Successfully registered";
				},
				(res) => {
					is_error = true;
					status_text = Rest.err_to_str(res);
				});
	}
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Register</p>
<TextBox label_text="username" bind:value={username}/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<TextBox label_text="display name" bind:value={displayname}/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<TextBox label_text="password" bind:value={password} is_password=true/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<TextBox label_text="repeat password" bind:value={password_repeat} is_password=true/>
<p class="suggestion_text"><button class="suggestion_button_link" onclick={() => setPage(0)}>Click here</button> to log in.</p>

<StatusBox text={status_text} is_error={is_error}/>

<Button text="Register" onClick={register}/>
</div>

<style>
	@import "login.css";
</style>
