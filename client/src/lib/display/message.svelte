<script>
	let {text, author, time_sent,
		show_ctx_menu} = $props();

	const padnum = (x, n) => x.toString().padStart(n, '0');
	const formatTimeMMSS = (date) => `${padnum(date.getMinutes(), 2)}:${padnum(date.getSeconds(), 2)}`;
</script>

<div class="message_panel hoverable" tabindex=0 role="group"
	oncontextmenu={(e) => {
		event.preventDefault();
		show_ctx_menu([e.clientX, e.clientY], "message");
}}>
	<div class="message_user_panel">
		<img class="user_avatar" src="$lib/assets/default_avatar.png" alt="avatar"/>
		<b>{author?.name}</b>
	</div>
	<div class="message_content_panel">
		<span class="message_time">{formatTimeMMSS(time_sent)}</span>
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
.message_content_panel {
	display: flex;
	align-items: end;

	font-size: 18px;
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
