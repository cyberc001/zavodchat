<script>
	import {asset} from '$app/paths';

	import Tabs from '$lib/control/tabs.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import Button from '$lib/control/button.svelte';
	import Autocomplete from '$lib/control/autocomplete.svelte';
	import Dialog from '$lib/control/dialog.svelte';

	import User from '$lib/rest/user.svelte.js';
	import BlockedUsers from '$lib/rest/blocked_users.js';
	import Friends from '$lib/rest/friends.js';
	import DM from '$lib/rest/dm.js';
	import Channel from '$lib/rest/channel.js';

	const {show_channel} = $props();

	let friends = Friends.get();
	let in_requests = Friends.get_requests(true);
	let out_requests = Friends.get_requests(false);

	let user_ac = $state();
	let user_to_add = $state();

	let friend_remove_confirm = $state();
	let user_to_remove = $state();

	const showDMChannel = (user_id, idx) => {
		let dm = DM.open(user_id);
		dm.notify_on_load(() => {
			const ch = Channel.get(dm.data[idx]);
			ch.notify_on_load(() => {
				show_channel(ch.data);
			});
		});
	};
</script>

{#snippet render_user(i, item)}
	<UserDisplay user={{data: item, loaded: true}}
	display_status={false}
	/>
{/snippet}

{#snippet tab_friends()}
<Dialog bind:this={friend_remove_confirm}
question={`Remove ${user_to_remove?.data?.name} from friends?`}
buttons={[{text: "Remove", action: () => Friends.remove_friend(user_to_remove.data.id)},
	  {text: "Cancel"}]}
>
</Dialog>
	{#if friends.loaded}
		<div style="display: flex; width: 100%">
			<Autocomplete bind:value={user_to_add} bind:this={user_ac}
			render_data={render_user}
			get_data={(index, range, asc, list_value_name) => User.get_range(index, range, asc, list_value_name)}
			--width="min(40vw, 350px)"/>
			<Button text="Add friend" --margin-left="10px"
			disabled={typeof user_to_add === "undefined"}
			onclick={() => Friends.accept_or_create_request(user_to_add.id, user_ac.reset)}/>
		</div>
		{#each friends.data as friend}
			<div class="friend_entry">
				<UserDisplay user={friend.user}/>

				<div class="friend_actions">
					{#if friend.user.loaded}
					<button class="transparent_button hoverable"
					onclick={() => showDMChannel(friend.user.data.id, 0)}>
						<img src={asset("icons/channel_text.svg")} alt="text friend" class="filter_icon_main" style="width: 32px"/>
					</button>

					<button class="transparent_button hoverable"
					onclick={() => showDMChannel(friend.user.data.id, 1)}>
						<img src={asset("icons/call.svg")} alt="call friend" class="filter_icon_main" style="width: 32px"/>
					</button>

					<button class="transparent_button hoverable"
					onclick={() => {
						user_to_remove = friend.user;
						friend_remove_confirm.show();
					}}>
						<img src={asset("icons/close.svg")} alt="remove friend" class="filter_icon_main" style="width: 32px"/>
					</button>
					{/if}
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
				<UserDisplay user={req} display_status={false}/>

				<div class="friend_actions">
				{#if req.loaded}
				{#if args.requests === in_requests}
					<button class="transparent_button hoverable"
					onclick={() => Friends.accept_or_create_request(req.data.id)}>
						<img src={asset("icons/add.svg")} alt="accept friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
					<button class="transparent_button hoverable"
					onclick={() => Friends.deny_request(req.data.id)}>
						<img src={asset("icons/close.svg")} alt="deny friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
					<button class="transparent_button hoverable"
					onclick={() => BlockedUsers.block_user(req.data.id)}>
						<img src={asset("icons/block.svg")} alt="block user" class="filter_icon_main" style="width: 32px"/>
					</button>
				{:else}
					<button class="transparent_button hoverable"
					onclick={() => Friends.deny_request(req.data.id)}>
						<img src={asset("icons/close.svg")} alt="cancel friend request" class="filter_icon_main" style="width: 32px"/>
					</button>
				{/if}
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
