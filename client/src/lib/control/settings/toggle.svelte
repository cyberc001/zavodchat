<script>
	let {label_text, states = "off_on",
		value = $bindable(0)} = $props();

	switch(states){
		case "off_on":
			states = [{letter: "O"}, {"letter": "I"}];
			break;
		case "off_default_on":
			states = [{letter: "O"}, {letter: "D"}, {"letter": "I"}];
			break;
	}

	const onPanelClick = (e) => {
		if(e.pointerType === ""){
			onLetterClick(e);
			return;
		}
		const a = e.offsetX / e.target.getBoundingClientRect().width;
		value = Math.round((states.length - 1) * a);
	};
	let letter_move_forward = true;
	const onLetterClick = (e) => {
		if(letter_move_forward){
			if(value === states.length - 1){
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
	};
</script>

<div class="settings_toggle_frame">
	<div style="position: relative">
		<button class="settings_control settings_letter_box"
			style={`left: ${value / (states.length - 1) * 100}%; transform: translate(${value === 0 ? 0
												   : value === states.length - 1 ? -100
												   : -50}%, -50%)`}
			onclick={onLetterClick}
		>
			{states[value].letter}
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
