<script>
	import FocusManager from '$lib/focus_manager.svelte';

	import Textbox from '$lib/control/textbox.svelte';

	import Emoji from '$lib/emoji.js';


	let {
		hide_picker = () => {},
		on_picked = (emoji) => {}
	} = $props();

	let self = $state();

	let emoji_search_textbox = $state();
	let emoji_search_value = $state("");
	$effect(() => {
		if(emoji_search_textbox)
			emoji_search_textbox.focus();
	});
</script>


<FocusManager element={self}
	onblur={() => {}/*hide_picker*/}
/>

{#snippet emoji_button(emoji)}
	<button class="transparent_button" onclick={() => {
		on_picked(emoji);
		hide_picker();
	}}>
		<img src={emoji.img_path} class="emoji_picker_emoji"/>
	</button>
{/snippet}

<div class="item emoji_picker_panel" bind:this={self}>
	<Textbox label_text="Emoji search" bind:value={emoji_search_value}
	bind:this={emoji_search_textbox}
	--margin-bottom="6px"/>
	<div class="emoji_picker_list">
		{#if emoji_search_value.length > 0}
			{#each Emoji.search(emoji_search_value) as e}
				{@render emoji_button(e)}
			{/each}		
		{:else}
			{#each Object.entries(Emoji.__emoji_groups) as group}
				<div class="emoji_group_title">{group[0]}</div>
				{#each group[1] as e}
					{@render emoji_button(e)}
				{/each}
			{/each}
		{/if}
	</div>
</div>

<style>
.emoji_picker_panel {
	display: flex;
	flex-direction: column;
	
	width: min(340px, 25vw);
	height: min(500px, 30vh);

	padding: 4px;
	text-align: left;
}
.emoji_picker_list {
	overflow-y: scroll;
}
.emoji_picker_emoji {
	height: 24px;
	padding: 1px 2px 1px 2px;
}
.emoji_group_title {
	font-size: 16px;
	margin-bottom: 4px;
}
</style>
