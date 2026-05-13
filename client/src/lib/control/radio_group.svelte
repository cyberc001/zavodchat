<script>
	let {
		entries = [],
		value = $bindable("")
	} = $props();
</script>

<div class="settings_radiogroup_frame settings_control">
	{#each entries as e}
		<div class="settings_radiobox_entry">
			<input type="radio"
				class="settings_radiobox"
				value={e.value} bind:group={value}
			/>
			{e.label}
		</div>
	{/each}
</div>

<style>
@import "style.css";
.settings_radiogroup_frame {
	display: flex;
	flex-direction: column;

	background: transparent;
	border-style: none;
}
.settings_radiogroup_frame:active {
	border-style: none;
}

.settings_radiobox_entry {
	display: flex;
	margin-bottom: 4px;
	align-items: flex-start;
}
.settings_radiobox_entry:last-child {
	margin-bottom: 0;
}
.settings_radiobox {
	margin-right: 6px;
	accent-color: var(--clr_text);
}
</style>
