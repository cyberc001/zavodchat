<script>
	let {text, author,
		show_ctx_menu} = $props();
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
		{text}
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

	font-size: 24px;
}
.message_content_panel {
	font-size: 18px;
}

.user_avatar {
	width: 32px;
	height: 32px;

	border-radius: 4px;
	margin-right: 8px;
}
</style>
