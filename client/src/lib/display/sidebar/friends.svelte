<script>
	import {asset} from '$app/paths';

	import Tabs from '$lib/control/tabs.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import Button from '$lib/control/button.svelte';
	import UserPicker from '$lib/control/user_picker.svelte';

	import Friends from '$lib/rest/friends.js';

	let friends = Friends.get();
	let in_requests = Friends.get_requests(true);
	let out_requests = Friends.get_requests(false);

	let user_picker = $state();
	let user_to_add = $state();
</script>

{#snippet tab_friends()}
	{#if friends.loaded}
		<div style="display: flex; width: 100%">
			<UserPicker bind:value={user_to_add} bind:this={user_picker}
			--width="min(40vw, 350px)"/>
			<Button text="Add friend" --margin-left="10px"
			disabled={typeof user_to_add === "undefined"}
			onclick={() => Friends.accept_or_create_request(user_to_add, user_picker.reset)}/>
		</div>
		{#each friends.data as friend}
			<div class="friend_entry">
				<UserDisplay user={friend.data}/>

				<div class="friend_actions">
					<button class="transparent_button hoverable"
					onclick={() => Friends.remove_friend(friend.data.id)}>
						<img src={asset("icons/close.svg")} alt="remove friend" class="filter_icon_main" style="width: 32px"/>
					</button>
				</div>
			</div>
		{/each}
	{:else}
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 32px"/>
	{/if}
{/snippet}

{#snippet tab_friend_requests(args)}
	{#if args.requests.loaded}
		{#each args.requests.data as req}
			<div class="friend_entry">
				<UserDisplay user={req.data} display_status={false}/>

				<div class="friend_actions">
				{#if args.requests === in_requests}
					<button class="transparent_button hoverable"
					onclick={() => Friends.accept_or_create_request(req.data.id)}>
						<img src={asset("icons/add.svg")} alt="accept friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
					<button class="transparent_button hoverable"
					onclick={() => Friends.deny_request(req.data.id)}>
						<img src={asset("icons/close.svg")} alt="deny friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
				{:else}
					<button class="transparent_button hoverable"
					onclick={() => Friends.deny_request(req.data.id)}>
						<img src={asset("icons/close.svg")} alt="cancel friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
				{/if}
				</div>
			</div>
		{/each}
	{:else}
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 32px"/>
	{/if}
{/snippet}


<Tabs tabs={[
	{name: "Friends", render: tab_friends},
	{name: "Incoming requests", render: tab_friend_requests, args: {requests: in_requests}},
	{name: "Outgoing requests", render: tab_friend_requests, args: {requests: out_requests}}
]}/>

<style>
.friend_entry {
	display: flex;

	width: 100%;
}

.friend_actions {
	margin-left: auto;
	padding: 0 8px 0 8px;
}
</style>
