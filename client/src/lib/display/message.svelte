<script>
	import Message from '$lib/rest/message.js';

	let {id, text, author, time_sent, time_edited,
		selected = false, status = Message.Status.None,
		show_ctx_menu} = $props();
	let is_edited = $derived(time_sent.getTime() !== time_edited.getTime());

	let status_msg = $derived(
		  typeof status == "string" ? status
		: status == Message.Status.Sending ? "Sending..."
		: status == Message.Status.Editing ? "Editing..."
		: status == Message.Status.Deleting ? "Deleting..."
		: ""
	);

	const padnum = (x, n) => x.toString().padStart(n, '0');
	const formatTimeHHMM = (date) => `${padnum(date.getHours(), 2)}:${padnum(date.getMinutes(), 2)}`;
</script>

<div id={"message_display_" + id} class={"message_panel hoverable" + (selected ? " selected" : "")} tabindex=0 role="group"
	oncontextmenu={(e) => {
		event.preventDefault();
		show_ctx_menu([e.clientX, e.clientY], "message");
}}>
	<div class="message_user_panel">
		<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
		<b>{author?.name}</b>
	</div>
	{#if status_msg !== ""}
		{#if typeof status === "string"}
			<div class="message_status_panel" style="background: var(--clr_bg_text_selection)">
				{status_msg}
			</div>
		{:else}
			<div class="message_status_panel">
				<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="margin-right: 5px"/>
				{status_msg}
			</div>
		{/if}
	{/if}
	<div class="message_content_panel" title={`Sent: ${time_sent}\nLast edited: ${is_edited ? time_edited : "never"}`}>
		<div>
			<div class="message_time">{formatTimeHHMM(time_sent)}</div>
			{#if is_edited}
				<div class="message_time">edited</div>
			{/if}
		</div>
		<span>{text}</span>
	</div>
</div>

<style>
.message_panel {
	width: fit-content;
	padding: 3px;
	margin: 3px 3px 9px 3px;
}
.message_user_panel {
	display: flex;
	align-items: center;

	margin-bottom: 6px;

	font-size: 24px;
}
.message_status_panel {
	display: inline-flex;
	align-items: center;
	padding: 4px 6px 4px 4px;
	background: var(--clr_bg_item);
}
.message_content_panel {
	display: flex;
	align-items: baseline;

	font-size: 18px;
	white-space: pre-line;
}
.message_time {
	margin-right: 8px;

	font-size: 14px;
	font-family: ui-monospace;
	color: var(--clr_text_secondary);
}

.user_avatar {
	width: 32px;
	height: 32px;

	border-radius: 4px;
	margin-right: 8px;
}
</style>
