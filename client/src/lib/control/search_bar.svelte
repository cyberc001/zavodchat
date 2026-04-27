<script>
	import FocusManager from '$lib/focus_manager.svelte';
	import DatePicker from '$lib/control/date_picker.svelte'

	import UserDisplay from '$lib/display/user.svelte';;
	import Autocomplete from '$lib/control/autocomplete.svelte';

	import User from '$lib/rest/user.svelte.js';

	let {server, // can be undefined if "server_user" is not used
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


{#snippet render_user(i, item)}
<UserDisplay user={{data: item, loaded: true}} server={server}
display_status={false}
/>
{/snippet}

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
				{:else if e.type === "server_user" || e.type === "user"}
					<Autocomplete render_data={render_user}
					get_data={(index, range, asc, list_value_name) => e.type === "server_user" ? 
												User.get_server_range(server.data.id, index, range, asc, list_value_name) :
												User.get_range(index, range, asc, list_value_name)}
					bind:value={
						() => params[e.param],
						(x) => {if(x) params[e.param] = x.id; else delete params[e.param];}
					}
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
