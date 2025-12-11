<script>
	import Group from "$lib/control/settings/group.svelte";
	import Textbox from "$lib/control/settings/textbox.svelte";
	import AvatarPicker from "$lib/control/settings/avatar_picker.svelte";	

	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";

	import Rest from "$lib/rest.js";
	import Server from "$lib/rest/server.js";

	let { server_id } = $props();

	let server = $state({});
	$effect(() => {
		if(server_id)
			server = Server.get(server_id);
	});

	// General
	let state_general = new SettingsTabState({name: "", avatar_file: null});

	let server_avatar_picker = $state();
	let server_avatar_url = $state("");

	$effect(() => {
		if(server.name){
			server_avatar_url = Server.get_avatar_path(server);
			state_general.set_all_states("name", server.name);
		}
	});

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				apply_changes: () => {
					let query = {};
					if(state_general.is_changed("name"))
						query.name = state_general.state.name;
					if(state_general.is_changed("avatar_file"))
						query.avatar = state_general.state.avatar_file;
					Server.change(server_id, query,
						() => state_general.apply_changes(),
						() => state_general.discard_changes());
				},
				discard_changes: () => state_general.discard_changes()
			}
		];
	}
</script>

{#snippet general(p)}
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker bind:this={server_avatar_picker}
		bind:file={state_general.state.avatar_file}
			bind:display_url={server_avatar_url}
		/>
		<div style="margin-left: 16px"></div>
	<Textbox label_text="Server name" bind:value={state_general.state.name} --width="363px"/>
	</div>
</Group>
{/snippet}
