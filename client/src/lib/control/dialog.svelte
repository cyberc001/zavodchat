<script>
	import Button from "$lib/control/button.svelte";

	let {question, buttons} = $props();
	let dialog = $state();
	
	export function show(){
		dialog.showModal();
	}
</script>

<dialog
	class="panel dialog_el"
	bind:this={dialog}
	onclick={(e) => {if(e.target === dialog) dialog.close();}}
>
	<div class="dialog_frame">
		<div class="dialog_question">{question}</div>
		<slot></slot>
		<div class="dialog_buttons">
			{#each buttons as butn}
				<Button text={butn.text} --margin="0 8px 0 0"
					disabled={butn.disabled}
					onclick={() => {if(butn.action) butn.action(); dialog.close();}}
				/>
			{/each}
		</div>
	</div>
</dialog>

<style>
.dialog_el {
	padding: 0;
}
.dialog_frame {
	padding: 8px;
}

.dialog_question {
	font-size: 28px;
	font-weight: bold;

	margin-bottom: 8px;
}
.dialog_buttons {
	margin-top: 8px;
}
</style>
