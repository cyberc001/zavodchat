<script>
	import {page} from '$app/state';
	import {goto} from '$app/navigation';
	import Server from '$lib/rest/server.js';
	import Invite from '$lib/rest/invite.js';
	import Button from '$lib/control/settings/button.svelte';
	
	let server = $state();
	Invite.get_nocache(page.params.slug,
				(data) => {
					server = data;
				},
				(err) => {
					server = null;
				});
	let error = $state("");
</script>

<div class="main">
	<div class="center_frame">
		{#if server === null}
			Invalid invite
		{:else if !server}
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
		{:else}
			{#if server.avatar}
				<img class="server_avatar" src={Server.get_avatar_path(server)} />
			{/if}
			{server.name}
			<Button text="Accept invite"
				--margin-top="16px" --margin-bottom="0px"
				onclick={() => {
					error = "";
					Invite.accept(page.params.slug,
							(res) => goto("/"),
							(err) => error = err.data);
				}}
			/>
			<div style="color: var(--clr_text_error); font-size: 18px">
				{error}
			</div>
		{/if}
	</div>
</div>

<style>
@import "../../../main.css";

.center_frame {
	margin: auto;
	
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;

	width: 250px;
	min-height: 200px;

	padding: 24px;

	border-style: solid;
	border-width: 2px;
	border-radius: 4px;
	border-color: #969A9F;

	font-size: 28px;
}

.server_avatar {
	width: 196px;
	height: 196px;
}
</style>