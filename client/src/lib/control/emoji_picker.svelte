<script>
	import FocusManager from '$lib/focus_manager.svelte';

	import Emoji from '$lib/emoji.js';


	let {
		hide_picker = () => {},
		on_picked = (emoji) => {}
	} = $props();

	let self = $state();
</script>


<FocusManager element={self}
	onblur={hide_picker}
/>

<div class="item emoji_picker_panel" bind:this={self}>
	{#each Emoji.search("") as e}
		<button class="transparent_button" onclick={() => {
			on_picked(e);
			hide_picker();
		}}>
			<img src={e.img_path} class="emoji_picker_emoji"/>
		</button>
	{/each}
</div>

<style>
.emoji_picker_panel {
}
.emoji_picker_emoji {
	height: 24px;
}
</style>
