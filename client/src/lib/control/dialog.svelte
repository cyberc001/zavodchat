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
				{#if butn.style === "negative"}
					<Button text={butn.text} 
						disabled={butn.disabled}
						onclick={() => {if(butn.action) butn.action(); dialog.close();}}
						--margin="0 8px 0 0"
						--background="var(--clr_bg_item_negative)"
						--border-color="var(--clr_border_item_negative)"
					/>
				{:else}
					<Button text={butn.text} 
						disabled={butn.disabled}
						onclick={() => {if(butn.action) butn.action(); dialog.close();}}
						--margin="0 8px 0 0"
					/>
				{/if}
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
	font-size: 22px;
	font-weight: bold;

	margin-bottom: 8px;
}
.dialog_buttons {
	margin-top: 8px;
}
</style>
