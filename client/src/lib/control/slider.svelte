<script>
	let {text, display_value = (value) => value,
		min = 0, max = 100, value = $bindable(0)} = $props();
</script>

<div style="display: flex; align-items: flex-start; width:var(--width, 100%);">
	<div style="margin-right: 8px">{text}</div>
	<input type="range" min={min} max={max} bind:value={value} class="slider"/>
	<div style="margin-left: 4px">{display_value(value)}</div>
</div>

<style>
@import "style.css";

.slider {
	accent-color: var(--clr_text);
}
</style>
