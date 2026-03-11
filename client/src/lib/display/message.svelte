<script>
	import {asset} from '$app/paths';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import File from '$lib/rest/file.js';
	import UserDisplay from '$lib/display/user.svelte';
	import MediaDisplay from '$lib/display/media.svelte';

	let {data,
		selected = false, selected_user = false, highlighted = false,
		show_ctx_menu, onclick_user, hide_profile} = $props();
	let is_edited = $derived(data.sent !== data.edited);

	let status_msg = $derived.by(() => {
		switch(data.status){
			case Message.Status.Sending:
				return "Sending...";
			case Message.Status.Editing:
				return "Editing...";
			case Message.Status.Deleting:
				return "Deleting...";
		}
		return status;
	});

	let self = $state();

	const padnum = (x, n) => x.toString().padStart(n, '0');
	const formatTimeHHMM = (date) => `${padnum(date.getHours(), 2)}:${padnum(date.getMinutes(), 2)}`;

	let shown_attachment = $state();
</script>

<div id={"message_display_" + data.id} class="message_panel" tabindex=0 role="group">
	<UserDisplay user={data.author?.data} user_roles={data.author_roles}
	message_id={data.id}
	display_status={false}
	selected={selected_user}
	onclick={onclick_user} hide_profile={hide_profile} show_ctx_menu={show_ctx_menu}
	/>
	{#if status_msg !== ""}
		{#if typeof status === "string"}
			<div class="message_status_panel" style="background: var(--clr_bg_text_selection)">
				{status_msg}
			</div>
		{:else}
			<div class="message_status_panel">
				<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="margin-right: 5px"/>
				{status_msg}
			</div>
		{/if}
	{/if}
	<div class={"message_content_panel hoverable" + (highlighted ? " highlighted" : (selected ? " selected" : ""))}
	style="anchor-name: --{"message_display_" + data.id}"
	role="listitem"
	title={`Sent: ${new Date(data.sent)}\nLast edited: ${is_edited ? new Date(data.edited) : "never"}`}
	bind:this={self}
	oncontextmenu={(e) => {
		event.preventDefault();
		show_ctx_menu(self, e, true);
	}}
	>
		<div>
			<div class="message_time">{formatTimeHHMM(new Date(data.sent))}</div>
			{#if is_edited}
				<div class="message_time">edited</div>
			{/if}
		</div>
		<div>
			<div>{data.text}</div>
			{#if data.attachments.length === 1}
			<button onclick={() => shown_attachment = data.attachments[0]} class="transparent_button unhoverable">
				<img src={File.get_attachment_url(data.attachments[0].content)} class="single_attachment_img"/>
			</button>
			{:else}
			<div>
				{#each data.attachments as att}
				<button onclick={() => shown_attachment = att} class="transparent_button unhoverable">
					<img src={File.get_attachment_url(att.content)} class="attachment_img"
					style="margin-right: 6px; cursor: pointer" />
				</button>
				{/each}
			</div>
			{/if}
		</div>
	</div>
</div>

{#if shown_attachment}
	<MediaDisplay close_media={() => shown_attachment = undefined}>
		<img class="fullscreen_media" src={File.get_attachment_url(shown_attachment.content)}/>
	</MediaDisplay>
{/if}

<style>
.message_panel {
	width: fit-content;
	padding: 3px;
	margin: 3px 3px 9px 3px;
}
.message_status_panel {
	display: inline-flex;
	align-items: center;
	padding: 4px 6px 4px 4px;
	background: var(--clr_bg_item);
}
.message_content_panel {
	display: flex;

	font-size: 18px;
	white-space: pre-line;
}
.message_time {
	margin-right: 8px;

	font-size: 14px;
	font-family: ui-monospace;
	color: var(--clr_text_secondary);
}

.single_attachment_img {
	max-height: 40vh;
	max-width: 50vw;
	object-fit: contain;

	cursor: pointer;
}
</style>
