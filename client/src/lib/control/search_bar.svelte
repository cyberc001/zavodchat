<script>
	import FocusManager from '$lib/focus_manager.svelte';
	import DatePicker from '$lib/control/date_picker.svelte';
	import ServerUserPicker from '$lib/control/server_user_picker.svelte';

	let {server_id, // can be undefined if "server_user" is not used
		elements,
		onsearch} = $props();

	let show_params = $state(false);
	let self = $state();

	let content = $state("");
	let params = {};
</script>

<FocusManager element={self}
	onblur={() => {
		show_params = false;
	}}
/>

<div class="search_bar_frame" bind:this={self}>
	<input autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false"
		class="settings_control" style="width:var(--width, 240px)"
		bind:value={content}
		onkeyup={(e) => {
			if(e.code === "Enter")
				onsearch(params);
		}}

		onfocus={() => {
			show_params = true;
		}}
	/>
	{#if show_params}
		<div class="item search_bar_params">
			{#each elements as e}
				{#if e.type === "date"}
					<DatePicker label_text={e.label} bind:value={params[e.param]}/>
				{:else if e.type === "server_user"}
					<ServerUserPicker label_text={e.label} bind:value={params[e.param]}
						server_id={server_id}
					/>
				{/if}
			{/each}
		</div>
	{/if}
</div>

<style>
@import "style.css";
.search_bar_frame {
	display: flex;
	flex-direction: column;
	margin-bottom: 12px;
}
.search_bar_params {
	padding: 6px 0 0 10px;
}
</style>
