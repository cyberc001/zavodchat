<script>
	import Group from "$lib/control/settings/group.svelte";
	import Textbox from "$lib/control/settings/textbox.svelte";
	import AvatarPicker from "$lib/control/settings/avatar_picker.svelte";	
	import Button from "$lib/control/settings/button.svelte";
	import Dialog from "$lib/control/dialog.svelte";

	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";

	import Rest from "$lib/rest.js";
	import Server from "$lib/rest/server.js";

	let { server_id } = $props();

	let server = $state({});
	$effect(() => {
		server = server_id ? Server.get(server_id) : {};
	});

	// General
	let state_general = new SettingsTabState({name: "", avatar: null});
	let delete_confirm = $state();

	let server_avatar_url = $state("");

	$effect(() => {
		if(server.name){
			server_avatar_url = Server.get_avatar_path(server);
			state_general.set_all_states("name", server.name);
		} else {
			server_avatar_url = "";
			state_general.set_all_states("name", "");
		}
	});

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				apply_changes: () => {
					Server.change(server_id, state_general.get_dict_of_changes(),
						() => state_general.apply_changes(),
						() => state_general.discard_changes());
				},
				discard_changes: () => state_general.discard_changes()
			}
		];
	}
</script>

{#snippet general(params, close_settings)}
<Dialog bind:this={delete_confirm}
question="Delete the server?"
buttons={[{text: "Delete", action: () => {Server.delete(server_id); close_settings();}},
	  {text: "Cancel"}]}
>
This cannot be reversed.
</Dialog>
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker
		bind:file={state_general.state.avatar}
		bind:display_url={server_avatar_url}
		/>
		<div style="margin-left: 16px"></div>
	<Textbox label_text="Server name" bind:value={state_general.state.name} --width="363px"/>
	</div>
</Group>
<Group name="Management">
	<Button text="Delete server" onclick={() => delete_confirm.show()}/>
</Group>
{/snippet}
