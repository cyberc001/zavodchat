<script>
	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import Select from '$lib/control/select.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Channel from '$lib/rest/channel.js';

	let { server_id, channel_id } = $props();

	let channel = Channel.get(channel_id);

	// General
	class ChannelTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!channel.loaded)
				return SettingsTabState.ChangesState.Loading;
		});

		apply_changes(){
			Channel.change(channel_id,
				super.get_dict_of_changes(),
				() => super.apply_changes(),
				() => super.discard_changes());
		}
	};
	let state_general = new ChannelTabState({name: "", type: Channel.Type.Text});

	$effect(() => {
		if(channel.loaded){
			state_general.set_default_state("name", channel.data.name);
			state_general.set_default_state("type", channel.data.type);
		}
	});

	export function tabs() {
		return [
			{name: "General", render: general, state: state_general}
		];
	}
</script>

{#snippet general()}
<Group name="General settings">
	<Textbox label_text="Channel name" bind:value={state_general.state.name}/>
	<Select label_text="Channel type" bind:value={state_general.state.type}
	options={[Channel.Type.Text, Channel.Type.Voice]}
	option_labels={["Text", "Voice"]}
	/>
</Group>
{/snippet}
