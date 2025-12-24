<script>
	import Group from "$lib/control/settings/group.svelte";
	import Textbox from "$lib/control/settings/textbox.svelte";
	import AvatarPicker from "$lib/control/settings/avatar_picker.svelte";	
	import Button from "$lib/control/settings/button.svelte";
	import OrderedList from "$lib/control/settings/ordered_list.svelte";
	import Dialog from "$lib/control/dialog.svelte";

	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";

	import Rest from "$lib/rest.js";
	import Server from "$lib/rest/server.js";
	import Role from "$lib/rest/role.js";

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

	// Roles
	let state_roles = new SettingsTabState({list: []});

	$effect(() => {
		if(server_id)
			state_roles.set_all_states("list", Role.get_list(server_id));
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
			},
			{ name: "Roles", render: roles, state: state_roles,
				apply_changes: () => {
					let role_list = state_roles.default_state.list.slice();
					let changes = [];

					console.log("begin", $state.snapshot(role_list));
					for(let i = 0; i < state_roles.state.list.length; ++i){
						const rol2 = state_roles.state.list[i];

						const j = role_list.findIndex((x) => x.id === rol2.id);
						if(j === -1)
							continue;

						const rol = role_list[j];
						
						const nextid = role_list[j - 1]?.id;
						const nextid2 = state_roles.state.list[i - 1]?.id;
						if(nextid !== nextid2){
							// Move rol2 to correct position in role_list
							role_list.splice(j, 1);
							const j2 = typeof nextid2 === "undefined" ? 0 : role_list.findIndex((x) => x.id === nextid2) + 1;
							role_list.splice(j2, 0, rol);
							console.log("iter", $state.snapshot(role_list));

							changes.push({id: rol.id, next_role_id: typeof nextid2 === "undefined" ? -1 : nextid2});
						}
					}
					console.log("changes", changes);

					let counter = 0; const counter_target = changes.length;
					for(const ch of changes){
						Role.change(server_id, ch.id, ch, () => {
							if(++counter === counter_target)
								state_roles.apply_changes();
						}, () => {state_roles.discard_changes();});
					}
				},
				discard_changes: () => {state_roles.discard_changes();}
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
	<div class="user_role_circle_big" style={Role.get_style(rol)}></div>
	<div style="margin-left: 6px; font-size: 18px">{rol.name}</div>
</div>
{/snippet}

{#snippet roles(params, close_settings)}
<OrderedList bind:items={state_roles.state.list}
check_drag={(dragged, dragged_idx) => { /* disallow to drag lowest (default) role */
					return dragged_idx !== state_roles.state.list.length - 1;}}
check_insert={(dragged, dragged_idx, hovered, hovered_idx) => { /* disallow to insert below lowest (default) role */
								return hovered_idx !== state_roles.state.list.length - 1;}}
render_item={role_item}
/>
{/snippet}
