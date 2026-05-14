<script>
	let {
		label_text = "", error = "",
		autocomplete = "off", is_password = false,
		value = $bindable(""),
		render_after
	} = $props();

	let input = $state();
	export function focus(){
		input.focus();
	}
</script>

<div class="settings_textbox_frame">
	<p class="settings_control_label">{label_text}</p>
	<div style="display: flex">
		<input class="settings_control" style="width:var(--width, 300px)"

			autocomplete={autocomplete}
			type={is_password ? "password" : "input"}

			autocorrect="off" autocapitalize="off" spellcheck="false"
			bind:value={value} bind:this={input}
		/>
		{#if render_after}
			{@render render_after()}
		{/if}
	</div>
	<p class="settings_control_label" style="color: var(--clr_text_error)">{error}</p>
</div>

<style>
@import "style.css";
.settings_textbox_frame {
	display: flex;
	flex-direction: column;
}
</style>
