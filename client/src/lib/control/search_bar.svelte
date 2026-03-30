<script>
	import FocusManager from '$lib/focus_manager.svelte';
	import DatePicker from '$lib/control/date_picker.svelte';
	import UserPicker from '$lib/control/user_picker.svelte';

	let {server_id, // can be undefined if "server_user" is not used
		elements,
		onsearch} = $props();

	let show_params = $state(false);
	let self = $state();

	let content = $state("");
	let params = $state({});

	export function reset(){
		content = "";
		params = {};
	}
</script>

<FocusManager element={self}
	onblur={() => {
		show_params = false;
		params = {};
	}}
/>

<div style="position: relative" bind:this={self}>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control" style="width:var(--width, 240px); margin-bottom:var(--margin-bottom, 12px)"
		bind:value={content}
		onkeyup={(e) => {
			if(e.code === "Enter"){
				let p = $state.snapshot(params);
				let words = content.split(" ").filter((x) => x.length > 0);
				if(words.length > 0)
					p.content = words;
				onsearch(p);
			}
		}}

		onfocus={() => {
			show_params = true;
		}}
	/>
	{#if show_params}
		<div class="item search_bar_params">
			{#each elements as e}
				{#if e.type === "date"}
					<DatePicker label_text={e.label} bind:value={params[e.param]}
						--margin-bottom="6px"
					/>
				{:else if e.type === "server_user"}
					<UserPicker label_text={e.label} bind:value={params[e.param]}
						server_id={server_id}
						--margin-bottom="6px"
					/>
				{:else if e.type === "user"}
					<UserPicker label_text={e.label} bind:value={params[e.param]}
						--margin-bottom="6px"
					/>
				{/if}
			{/each}
		</div>
	{/if}
</div>

<style>
@import "style.css";
.search_bar_params {
	position: absolute;
	z-index: 1000;
	top: 100%;
	/* same width as parent */
	left: 0;
	right: 0;

	padding: 6px 0 0 6px;
	border-style: solid;
	border-radius: 4px;
}
</style>
