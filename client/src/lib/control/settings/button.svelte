<script>
	const {text, onclick, disabled = false} = $props();
</script>

<button class="settings_button" onclick={onclick} {disabled}>{text}</button>

<style>
.settings_button {
	font-size: 20px;
	color: #F6F5F5;

	background: var(--clr_bg_item);

	border-color: var(--clr_border_item);
	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}

.settings_button:disabled {
	background: var(--clr_bg_panel);
	pointer-events: none;
}

.settings_button:focus {
	outline: none;
	border-color: var(--clr_border_focus);
}
.settings_button:active {
	border-style: outset;
}
</style>
