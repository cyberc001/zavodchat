<script>
	import DatePicker from '$lib/control/date_picker.svelte';

	let {elements} = $props();

	let content = $state("");
	let params = $state({});
</script>

<div class="search_bar_frame">
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="control" style="width:var(--width, 512px)"
		bind:value={content}/>
	{#each elements as e}
		{#if e.type === "date"}
			<DatePicker label=e.label value={params[e.param]}/>
		{/if}
	{/each}
</div>

<style>
@import "style.css";
.search_bar_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
</style>
