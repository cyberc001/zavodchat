<script>
	import Group from '$lib/control/settings/group.svelte';
	import Textbox from '$lib/control/settings/textbox.svelte';
	import Select from '$lib/control/settings/select.svelte';

	import CreateTabState from '$lib/control/create_tab_state.svelte.js';

	import Util from '$lib/util.js';
	import Channel from '$lib/rest/channel.js';

	let { server_id } = $props();

	// General
	class CreateTabStateGeneral extends CreateTabState {
		valid = $derived(this.state.name.length > 0);
	};
	let state_general = new CreateTabStateGeneral({name: "", type: Channel.Type.Text});

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				create: (close_settings) => {
					Channel.create(server_id, state_general.state,
						close_settings,
						() => {});
				}
			}
		];
	}
</script>

{#snippet general()}
<Group name="Channel settings">
	<Textbox label_text="Channel name" bind:value={state_general.state.name}/>
	<Select label_text="Channel type" bind:value={state_general.state.type}
	options={[0, 1]}
	option_labels={["Text", "Voice"]}
	/>
</Group>
{/snippet}
