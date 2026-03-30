<script>
	import {asset} from '$app/paths';
	import User from '$lib/rest/user.svelte.js';	

	const {channel,
		show_channel} = $props();

	let user = $derived(User.get(channel.user_id));
</script>

<div class="dm_frame">
	<button class="transparent_button hoverable"
		style="max-width: 100%"
		onclick={() => show_channel(channel)}
	>
		{#if !user.loaded}
			<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 32px"/>
		{:else}
			<button class="transparent_button hoverable"
			onclick={() => show_channel(channel.id)}>
			<div style="display: flex; align-items: center">
				<img class="user_avatar" src={User.get_avatar_path(user)} alt="avatar"/>
				<div class="dm_channel_frame">
					<b class="dm_text">{user.data.name}</b>
					<span class="dm_text">{channel.last_message?.text}</span>
				</div>
			</div>
			</button>
		{/if}
	</button>
</div>

<style>
.dm_frame {
	box-sizing: border-box;
	width: 100%;
	padding: 12px;
}
.dm_text {
	white-space: nowrap;
	overflow-x: hidden;
	text-overflow: ellipsis;
}
.user_avatar {
	width: 36px;
	height: 36px;
}

.dm_channel_frame {
	margin-left: 8px;
	overflow-x: hidden;

	display: flex;
	flex-direction: column;
	text-align: left;
}
</style>
