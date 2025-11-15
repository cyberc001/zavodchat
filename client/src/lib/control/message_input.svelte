<script>
	let { hint = "", max_rows = 5, value = $bindable(""),
		actions = [],
		onsend } = $props();

	let textarea_rows = $state(1);
	$effect(() => {
		let text_rows = 1;
		for(let i = 0; i < value.length; ++i)
			if(value[i] == '\n')
				++text_rows;
		textarea_rows = Math.min(text_rows, max_rows);
	});
	const textarea_onkeyup = (e) => {
		if(e.code === "Enter" && e.ctrlKey
		&& value.length > 0){
			onsend(value);
			value = "";
		}
	}
</script>

<div class="message_input">
	<div class="message_input_center_panel">
		<div class="message_input_actions">
			{#each actions as a}
				<button class="item message_input_action_button" onclick={a.func}>{a.text}</button>
			{/each}
		</div>
		<textarea class="item message_input_textarea" rows={textarea_rows}
			bind:value
			onkeyup={textarea_onkeyup}/>
	</div>
</div>

<style>
.message_input {
	width: 100%;
	text-align: center;
	margin-top: auto;
}
.message_input_center_panel {
	display: inline-block;
	width: 80%;
}

.message_input_actions {
	width: 100%;
	display: inline-block;
	text-align: left;
	margin-bottom: 6px;
}
.message_input_action_button {
	display: inline-block;

	border-color: var(--clr_border_item);
	border-radius: 4px;
	border-style: solid;

	font-size: 16px;
	color: var(--clr_text);
}
.message_input_action_button:active {
	background: var(--clr_bg_selected);
}


.message_input_textarea {
	width: 100%;
	resize: none;

	border-color: var(--clr_border_item);
	border-style: solid;
	border-width: 2px;

	color: var(--clr_text);
	font-size: 16px;
}
</style>
