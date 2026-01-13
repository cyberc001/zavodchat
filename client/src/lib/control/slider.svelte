<script>
	let {text, display_value = (value) => value,
		min = 0, max = 100, value = $bindable(0)} = $props();
</script>

<div style="display: flex; width:var(--width, 100%);">
	<div style="margin-right: 8px">{text}</div>
	<input type="range" min={min} max={max} bind:value={value}/>
	<div style="margin-right: 8px">{display_value(value)}</div>
</div>

<style>
@import "style.css";
</style>
