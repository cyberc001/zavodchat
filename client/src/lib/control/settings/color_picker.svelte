<script>
	let {label_text,
		value = $bindable("")} = $props();
</script>

<div class="settings_color_picker_frame">
	<p class="settings_control_label">{label_text}</p>
	<input type="color" class="settings_control" style="width:var(--width, 64px)"
	bind:value={value}/>
</div>

<style>
@import "style.css";
.settings_color_picker_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
</style>
