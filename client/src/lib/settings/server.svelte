<script>
	import Util from '$lib/util.js';

	import Group from '$lib/control/settings/group.svelte';
	import Textbox from '$lib/control/settings/textbox.svelte';
	import AvatarPicker from '$lib/control/settings/avatar_picker.svelte';
	import Button from '$lib/control/settings/button.svelte';
	import OrderedList from '$lib/control/settings/ordered_list.svelte';
	import ColorPicker from '$lib/control/settings/color_picker.svelte';
	import Toggle from '$lib/control/settings/toggle.svelte';
	import Dialog from '$lib/control/dialog.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Server from '$lib/rest/server.js';
	import Role from '$lib/rest/role.js';

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
		if(server.loaded){
			server_avatar_url = Server.get_avatar_path(server.data);
			state_general.set_default_state("name", server.data.name);
		} else {
			server_avatar_url = "";
			state_general.set_all_states("name", "");
		}
	});

	// Roles
	let state_roles = new SettingsTabState({list: []});

	let role_list_selected_idx = $state(-1);

	let prev_server_id = $state(server_id);
	$effect(() => {
		if(server_id !== prev_server_id)
			role_list_selected_idx = -1;

		if(server_id)
			state_roles.set_default_state("list", Role.get_list(server_id).data);
		else
			state_roles.set_all_states("list", []);

		prev_server_id = server_id;
	});

	function perm_to_toggle_value(x){
		if(++x > 2)
			x = 0;
		return x;
	}
	function toggle_value_to_perm(x){
		if(--x < 0)
			x = 2;
		return x;
	}
	function role_perm_get_fabric(set_number, set_idx){
		return () => {
			return perm_to_toggle_value((state_roles.state.list[role_list_selected_idx]["perms" + set_number] >> (set_idx * 2)) & 0x3);
		};
	}
	function role_perm_set_fabric(set_number, set_idx){
		return (x) => {
			x = toggle_value_to_perm(x);
			state_roles.state.list[role_list_selected_idx]["perms" + set_number] &= ~(0x3 << (set_idx * 2));
			state_roles.state.list[role_list_selected_idx]["perms" + set_number] |= x << (set_idx * 2); 
		};
	}

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				apply_changes: () => {
					Server.change(server_id, state_general.get_dict_of_changes(),
						() => state_general.apply_changes(),
						() => state_general.discard_changes());
				},
				discard_changes: () => state_general.discard_changes()
			},
			{ name: "Roles", render: roles, state: state_roles,
				apply_changes: () => {
					Role.change_list(server_id, state_roles.state.list,
								() => state_general.apply_changes(),
								() => state_general.discard_changes());
				},
				discard_changes: () => {
					role_list_selected_idx = -1;
					state_roles.discard_changes();
				}
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


{#snippet role_item(rol)}
<div style="display: flex; align-items: center; padding: 3px 0 3px 3px">
	<div class="user_role_circle_big" style={Role.get_background_style(rol)}></div>
	<div style="margin-left: 6px; font-size: 18px">{rol.name}</div>
</div>
{/snippet}

{#snippet roles(params, close_settings)}
<Group name="Role priority">
	<OrderedList bind:items={state_roles.state.list} bind:selected_idx={role_list_selected_idx}
	check_drag={(dragged, dragged_idx) => { /* disallow to drag lowest (default) role */
						return dragged_idx !== state_roles.state.list.length - 1;}}
	check_insert={(dragged, dragged_idx, hovered, hovered_idx) => { /* disallow to insert below lowest (default) role */
									return hovered_idx !== state_roles.state.list.length - 1;}}
	render_item={role_item}
	/>
	<div style="margin-bottom: 12px"></div>
	<Button text="Create role" onclick={() => {
		state_roles.state.list.splice(state_roles.state.list.length - 1, 0, Role.get_dummy_role());
		role_list_selected_idx = state_roles.state.list.length - 2;
	}}/>
	{#if role_list_selected_idx > -1 && role_list_selected_idx < state_roles.state.list.length - 1}
		<Button text="Delete role" onclick={() => {
			state_roles.state.list.splice(role_list_selected_idx, 1);
			role_list_selected_idx = -1;
		}}/>
	{/if}
</Group>
<Group name="Role settings">
{#if role_list_selected_idx > -1}
	<Textbox label_text="Name" bind:value={state_roles.state.list[role_list_selected_idx].name} --width="363px"/>
	<ColorPicker label_text="Color" bind:value={state_roles.state.list[role_list_selected_idx].color}/>
	Permissions
	<Toggle label_text="Send messages" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 0), role_perm_set_fabric(1, 0)}
	/>
	<Toggle label_text="Delete messages of other users" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 1), role_perm_set_fabric(1, 1)}
	/>
	<Toggle label_text="Manage server" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 2), role_perm_set_fabric(1, 2)}
	/>
	<Toggle label_text="Kick users" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 3), role_perm_set_fabric(1, 3)}
	/>
	<Toggle label_text="Manage bans" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 4), role_perm_set_fabric(1, 4)}
	/>
	<Toggle label_text="Manage channels" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 5), role_perm_set_fabric(1, 5)}
	/>
	<Toggle label_text="Manage invites" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 6), role_perm_set_fabric(1, 6)}
	/>
	<Toggle label_text="Speak in voice channels" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 7), role_perm_set_fabric(1, 8)}
	/>
	<Toggle label_text="Manage roles" states="off_default_on"
		bind:value={role_perm_get_fabric(1, 8), role_perm_set_fabric(1, 8)}
	/>
{/if}
</Group>
{/snippet}
