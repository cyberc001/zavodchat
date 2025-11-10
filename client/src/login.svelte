<script>
	import TextBox from '$lib/control/login/textbox.svelte';
	import Button from '$lib/control/login/button.svelte';
	import StatusBox from '$lib/control/login/statusbox.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';
	import Server from '$lib/rest/server.js';

	let { setPage } = $props();

	let username = $state(""), password = $state("");

	let error_text = $state("");

	// Check if authentificated
	Server.get_list(() => setPage(2),
			(err) => {
					if(err.status !== 400 && err.status !== 401)
						window.alert(err.status + " " + err.data);
				}
	);
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Log in</p>
<TextBox label_text="username" bind:value={username}/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<TextBox label_text="password" bind:value={password} is_password=true/>
<p class="suggestion_text"><button class="suggestion_button_link" onclick={() => setPage(1)}>Click here</button> to set up an account.</p>

<StatusBox text={error_text}/>

<Button text="Log in" onClick={() => Auth.login(username, password,
					() => setPage(2),
					(res) => error_text = Rest.err_to_str(res))}/>

</div>

<style>
	@import "login.css";
</style>
