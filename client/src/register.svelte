<script>
	import TextBox from '$lib/control/login/textbox.svelte';
	import Button from '$lib/control/login/button.svelte';
	import ErrorBox from '$lib/control/login/errorbox.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';

	let { setPage } = $props();

	let username = $state(""), displayname = $state(""),
		password = $state(""), password_repeat = $state("");

	let error_text = $state("");

	
	let register = function(){
		if(password != password_repeat){
			error_text = "Repeated password mismatch";
			return;
		}

		Auth.register(username, displayname, password,
				() => error_text = "Success",
				(res) => error_text = Rest.err_to_str(res));
	}
	Rest.host = "https://localhost";
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Register</p>
<TextBox label_text="username" bind:value={username}/>
<p style="margin: 0; margin-bottom: 6px;"/>
<TextBox label_text="display name" bind:value={displayname}/>
<p style="margin: 0; margin-bottom: 6px;"/>
<TextBox label_text="password" bind:value={password} is_password=true/>
<p style="margin: 0; margin-bottom: 6px;"/>
<TextBox label_text="repeat password" bind:value={password_repeat} is_password=true/>
<p class="suggestion_text"><a on:click={() => setPage(0)}>Click here</a> to log in.</p>

<ErrorBox text={error_text}/>

<Button text="Register" onClick={register}/>
</div>

<style>
	@import "login.css";
</style>
