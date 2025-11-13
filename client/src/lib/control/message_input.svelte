<script>
	let { hint = "", max_rows = 5, value = $bindable(""),
		onsend } = $props();

	let textarea_rows = $state(1);
	const resize_input = (e) => {
		const textarea = e.target;
		let text_rows = 1;
		for(let i = 0; i < textarea.value.length; ++i)
			if(textarea.value[i] == '\n')
				++text_rows;
		textarea_rows = Math.min(text_rows, max_rows);
	};
	const textarea_onkeyup = (e) => {
		if(e.code === "Enter" && e.ctrlKey)
			onsend(value);
	}
</script>

<div class="message_input">
	<textarea class="item message_input_textarea" rows={textarea_rows}
		bind:value
		oninput={resize_input}
		onkeyup={textarea_onkeyup}/>
</div>

<style>
.message_input {
	width: 100%;
	text-align: center;
}
.message_input_textarea {
	width: 80%;
	resize: none;

	border-color: var(--clr_border_item);
	border-style: solid;
	border-width: 2px;

	color: var(--clr_text);
	font-size: 16px;
}
</style>
