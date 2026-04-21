<script>
	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import Select from '$lib/control/select.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Util from '$lib/util.js';
	import Channel from '$lib/rest/channel.js';

	let {server_id,
		tabs = $bindable()} = $props();

	// General
	class ChannelTabState extends SettingsTabState {
		type = "create";
		changes_override = $derived.by(() => {
			if(this.state.name.length === 0)
				return SettingsTabState.ChangesState.Invalid;
		});

		apply_changes(close_settings){
			Channel.create(server_id, state_general.state,
					close_settings, () => {});
		}
	};
	let state_general = new ChannelTabState({name: "", type: Channel.Type.Text});

	tabs = [
			{name: "General", render: general, state: state_general}
	];
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
