<script>
	import TextBox from '$lib/control/login/textbox.svelte';
	import Button from '$lib/control/login/button.svelte';
	import ErrorBox from '$lib/control/login/errorbox.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';

	let { setPage } = $props();

	let username = $state(""), password = $state("");

	let error_text = $state("");

	Rest.host = "https://localhost";
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Log in</p>
<TextBox label_text="username" bind:value={username}/>
<p style="margin: 0; margin-bottom: 6px;"/>
<TextBox label_text="password" bind:value={password} is_password=true/>
<p class="suggestion_text"><a on:click={() => setPage(1)}>Click here</a> to set up an account.</p>

<ErrorBox text={error_text}/>

<Button text="Log in" onClick={() => Auth.login(username, password,
					() => error_text = "Success",
					(res) => error_text = Rest.err_to_str(res))}/>
</div>

<style>
	@import "login.css"
</style>
