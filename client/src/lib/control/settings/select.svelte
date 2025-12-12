<script>
	let {label_text, options = [], option_labels,
		value = $bindable("")} = $props();
</script>

<div class="settings_select_frame">
	<p class="settings_select_label">{label_text}</p>
	<select class="settings_select" bind:value={value}>
		{#if typeof option_labels === "undefined"}
			{#each options as o}
				<option value={o}>{o}</option>
			{/each}
		{:else}
			{#each options as o, i}
				<option value={o}>{option_labels[i]}</option>
			{/each}
		{/if}
	</select>
</div>

<style>
.settings_select_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
.settings_select {
	font-size: 20px;
	color: var(--clr_text);

	box-sizing: border-box;
	width: var(--width, 512px);

	background: var(--clr_bg_item);

	border-color: var(--clr_border_item);
	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}
.settings_select:focus {
	outline: none;

	border-color: var(--clr_border_focus);
	border-style: solid;
}

.settings_select_label {
	text-align: left;
	margin: 0;
	margin-bottom: 4px;
	font-size: 16px;
}
</style>
