<script>
	import {untrack} from 'svelte';

	let {label_text,
		states = "off_on", forbid_states = {},
		value = $bindable(0)
	} = $props();

	$effect(() => {
		switch(states){
			case "off_on":
				untrack(() => states = [{letter: "O"}, {"letter": "I"}]);
				break;
			case "off_default_on":
				untrack(() => states = [{letter: "O"}, {letter: "D"}, {"letter": "I"}]);
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
</script>

<div class="settings_toggle_frame">
	<div style="position: relative">
		<button class="settings_control settings_letter_box"
			style={`left: ${value / states_last_i * 100}%; transform: translate(${value === 0 ? 0
												   : value === states_last_i ? -100
												   : -50}%, -50%)`}
			onclick={onLetterClick}
		>
			{letter}
		</button>
		<button class="settings_control settings_toggle_button"
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
}
.settings_toggle_button {
	width: 64px;
	height: 28px;
}
.settings_toggle_label {
	margin-left: 8px;
}

.settings_letter_box {
	padding: 0;
	width: 22px;
	height: 130%;

	position: absolute;
	align-content: center;
	top: 50%;
}
</style>
