<script>
	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import Select from '$lib/control/select.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Channel from '$lib/rest/channel.js';

	let { server_id, channel_id } = $props();

	let channel = $state({});
	$effect(() => {
		channel = channel_id ? Channel.get(channel_id) : {};
	});

	// General
	let state_general = new SettingsTabState({name: "", type: Channel.Type.Text});

	$effect(() => {
		if(channel.loaded){
			state_general.set_default_state("name", channel.data.name);
			state_general.set_default_state("type", channel.data.type);
		} else {
			state_general.set_all_states("name", "");
			state_general.set_all_states("type", Channel.Type.Text);
		}
	});

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				apply_changes: () => {
					Channel.change(channel_id,
						state_general.get_dict_of_changes(),
						() => state_general.apply_changes(),
						() => state_general.discard_changes());
				},
				discard_changes: () => state_general.discard_changes()
			}
		];
	}
</script>

{#snippet general()}
<Group name="General settings">
	<Textbox label_text="Channel name" bind:value={state_general.state.name}/>
	<Select label_text="Channel type" bind:value={state_general.state.type}
	options={[0, 1]}
	option_labels={["Text", "Voice"]}
	/>
</Group>
{/snippet}
