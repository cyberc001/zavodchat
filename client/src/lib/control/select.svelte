<script>
	let {label_text, options = [], option_labels,
		value = $bindable("")} = $props();

	if(options.length > 0)
		value = options[0];
	</script>

<div class="settings_select_frame">
	<p class="settings_control_label">{label_text}</p>
	<select class="settings_control" style="width:var(--width, 512px); margin-bottom:var(--margin-bottom, 12px)"
	bind:value={value}>
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
@import "style.css";
.settings_select_frame {
	display: flex;
	flex-direction: column;
}
</style>
