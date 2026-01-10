<script>
	import {goto} from '$app/navigation';

	import Textbox from '$lib/control/textbox.svelte';
	import Button from '$lib/control/button.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';
	
	import NotifDisplay from '$lib/display/notif.svelte';

	let username = $state(""), password = $state("");

	let error = $state("");
</script>

<div class="center_frame">
<p style="margin: 0; margin-bottom: 20px;">Log in</p>
<Textbox label_text="username" bind:value={username} error={error}/>
<p style="margin: 0; margin-bottom: 6px;"></p>
<Textbox label_text="password" bind:value={password} is_password=true/>
<p class="suggestion_text"><button class="suggestion_button_link" onclick={() => goto("/register")}>Click here</button> to set up an account.</p>

<Button text="Log in" --margin-bottom="0px"
	onclick={() => {
			error = "";
			Auth.login(username, password,
				() => goto("/"),
				(res) => error = Rest.err_to_str(res));
			}}
/>

</div>

<NotifDisplay/>


<style>
	@import "../main.css";
	@import "../auth.css";
</style>
