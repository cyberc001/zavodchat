<script>
	import TextBox from '$lib/control/login/textbox.svelte';
	import Button from '$lib/control/login/button.svelte';
	import ErrorBox from '$lib/control/login/errorbox.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';
	import Servers from '$lib/rest/servers.js';

	let { setPage } = $props();

	let username = $state(""), password = $state("");

	let error_text = $state("");

	Rest.host = "https://127.0.0.1";
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Log in</p>
<TextBox label_text="username" bind:value={username}/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<TextBox label_text="password" bind:value={password} is_password=true/>
<p class="suggestion_text"><button class="suggestion_button_link" onclick={() => setPage(1)}>Click here</button> to set up an account.</p>

<ErrorBox text={error_text}/>

<Button text="Log in" onClick={() => Auth.login(username, password,
					() => {},
					(res) => error_text = Rest.err_to_str(res))}/>
<Button text="get servers" onClick={() => Servers.get_list((res) => console.log(res), () => {})}/>

</div>

<style>
	@import "login.css";
</style>
