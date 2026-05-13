<script>
	import {untrack} from 'svelte';

	let {label_text,
		states = "off_on", forbid_states = {},
		value = $bindable(0)
	} = $props();

	$effect(() => {
		switch(states){
			case "off_on":
				untrack(() => states = [
					{letter: "O", background: "var(--clr_bg_item_negative)", border_color: "var(--clr_border_item_negative)"},
					{letter: "I", background: "var(--clr_bg_item_positive)", border_color: "var(--clr_border_item_positive)"}
				]);
				break;
			case "off_default_on":
				untrack(() => states = [
					{letter: "O", background: "var(--clr_bg_item_negative)", border_color: "var(--clr_border_item_negative)"},
					{letter: "D"},
					{letter: "I", background: "var(--clr_bg_item_positive)", border_color: "var(--clr_border_item_positive)"}
				]);
				break;
		}
	});
	
	const onPanelClick = (e) => {
		if(e.pointerType === ""){
			onLetterClick(e);
			return;
		}
		const a = e.offsetX / e.target.getBoundingClientRect().width;
		value = Math.round((states.length - 1) * a);

		while(forbid_states[value])
			onLetterClick();
	};
	let letter_move_forward = true;
	const onLetterClick = (e) => {
		if(letter_move_forward){
			if(value === states_last_i){
				--value;
				letter_move_forward = false;
			} else
				++value;
		} else{
			if(value === 0){
				++value;
				letter_move_forward = true;
			} else
				--value;
		}

		while(forbid_states[value])
			onLetterClick();
	};

	let states_last_i = $derived(Array.isArray(states) ? states.length - 1 : -1);

	let letter = $derived(typeof(states[value]) === "object" ? states[value].letter : '?');
	let button_style = $derived.by(() => {
		if(typeof(states[value]) !== "object")
			return "";
		let style = "";
		if(typeof(states[value].background) !== "undefined")
			style += `background: ${states[value].background};`;
		if(typeof(states[value].border_color) !== "undefined")
			style += `border-color: ${states[value].border_color};`;
		return style;
	});
</script>

<div class="settings_toggle_frame">
	<div class="settings_toggle_toggler">
		<button class="settings_control settings_letter_box"
			style={`left: ${value / states_last_i * 100}%; transform: translate(${value === 0 ? 0
												   : value === states_last_i ? -100
												   : -50}%);` + button_style}
			onclick={onLetterClick}
		>
			{letter}
		</button>
		<button class="settings_control settings_toggle_button"
			style={button_style}
			onclick={onPanelClick}
		>
		</button>
	</div>
	<p class="settings_toggle_label">{label_text}</p>
</div>

<style>
@import "style.css";
.settings_toggle_frame {
	display: flex;
	align-items: center;

	margin: var(--margin, 0 0 0 0);
}

.settings_toggle_toggler {
	display: flex;
	position: relative;
}
.settings_toggle_button {
	margin: 0;
	width: 64px;
	height: 26px;
}
.settings_toggle_label {
	margin-left: 8px;
}

.settings_letter_box {
	padding: 0;
	margin: 0;
	width: 22px;
	height: 100%;

	position: absolute;
	align-content: center;
	top: 0%;
}
</style>
