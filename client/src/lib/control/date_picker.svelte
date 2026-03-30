<script>
	import Util from '$lib/util.js';

	let {label_text,
		value = $bindable()} = $props();
</script>

<div class="settings_date_picker_frame">
	<p class="settings_control_label">{label_text}</p>
	<input type="date" class="settings_control" style="width:var(--width, 200px)"
	bind:value={
		() => {
			if(!value)
				return "";
			const d = new Date(value);
			return `${Util.padded(d.getFullYear(), 4)}-${Util.padded(d.getMonth() + 1, 2)}-${Util.padded(d.getDate(), 2)}`;
		},
		(x) => value = x.length === 0 ? undefined : new Date(x).toISOString()
	}
/>
</div>

<style>
@import "style.css";
.settings_date_picker_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
</style>
