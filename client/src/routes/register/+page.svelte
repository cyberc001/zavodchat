<script>
	import {goto} from '$app/navigation';

	import Textbox from '$lib/control/textbox.svelte';
	import Button from '$lib/control/button.svelte';
	import Rest from '$lib/rest.js';
	import Auth from '$lib/rest/auth.js';
	
	import NotifDisplay from '$lib/display/notif.svelte';
	import Notifs from '$lib/notifs.svelte.js';

	let username = $state(""), displayname = $state(""),
		password = $state(""), password_repeat = $state("");

	let passwords_match = $derived(password === password_repeat);
	
	let register = function(){
		Auth.register(username, displayname, password,
				() => {
					Notifs.add_notif("Successfuly registered", Notifs.Types.Normal);
				},
				() => {}
		);
	}
</script>

<div class="center_frame">
	<p style="margin: 0; margin-bottom: 12px;">Register</p>
	<Textbox label_text="username" bind:value={username}
		--margin="0 0 6px 0"/>
	<Textbox label_text="display name" bind:value={displayname}
		--margin="0 0 6px 0"/>
	<Textbox label_text="password" bind:value={password} is_password=true
		--margin="0 0 6px 0"/>
	<Textbox label_text="repeat password" bind:value={password_repeat} is_password=true
		error={passwords_match ? "" : "Passwords do not match"}
		--margin="0 0 6px 0"
	/>
	<p class="suggestion_text">
		<button class="suggestion_button_link" onclick={() => goto("/login")}>Click here</button> to log in.
	</p>
	
	<div>
		<Button text="Register" onclick={register} disabled={!passwords_match}/>
	</div>
</div>

<NotifDisplay/>


<style>
	@import "../main.css";
	@import "../auth.css";
</style>
