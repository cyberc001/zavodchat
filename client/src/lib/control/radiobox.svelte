<script>
	let {text = "",
		value, group = $bindable("")} = $props();
</script>

<div class="settings_radiobox_frame">
	<div style="display: flex; width:var(--width, 512px)">
		<input type="radio" name={name}
		class="settings_control settings_radiobox"
		value={value} bind:group={group}/>
		{text}
	</div>
</div>

<style>
@import "style.css";
.settings_radiobox_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
.settings_radiobox {
	margin-right: 6px;
	accent-color: var(--clr_text);
}
</style>
