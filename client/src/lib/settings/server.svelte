<script>
	import Util from '$lib/util.js';

	import Group from '$lib/control/settings/group.svelte';
	import Textbox from '$lib/control/settings/textbox.svelte';
	import AvatarPicker from '$lib/control/settings/avatar_picker.svelte';
	import Button from '$lib/control/settings/button.svelte';
	import OrderedList from '$lib/control/settings/ordered_list.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import ColorPicker from '$lib/control/settings/color_picker.svelte';
	import Select from '$lib/control/settings/select.svelte';
	import Toggle from '$lib/control/settings/toggle.svelte';
	import Dialog from '$lib/control/dialog.svelte';
	import UserDisplay from '$lib/display/user.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Server from '$lib/rest/server.js';
	import Role from '$lib/rest/role.js';
	import Ban from '$lib/rest/ban.js';

	let { server_id } = $props();

	let server = $state({});
	$effect(() => {
		server = server_id ? Server.get(server_id) : {};
	});

	// General
	let state_general = new SettingsTabState({name: "", avatar: ""});
	let delete_confirm = $state();
	let avatar_picker = $state();

	$effect(() => {
		if(server.loaded){
			state_general.set_all_states("avatar", Server.get_avatar_path(server.data));
			state_general.set_all_states("name", server.data.name);
		} else {
			state_general.set_all_states("avatar", "");
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


	// Bans
	let state_bans = new SettingsTabState({changed_bans: {}});

	const BanState = {
		Unchanged: 0,
		Changed: 1,
		Removed: 2
	};

	let ban_list = $state();
	let ban_duration_units = $state("");
	let ban_button_text = $derived.by(() => {
		if(ban_select_change?.state === BanState.Removed)
			return "Ban";
		return "Unban";
	});

	let ban_select_id = $state(-1);
	let ban_select_change = $derived(state_bans.state.changed_bans[ban_select_id]);
	$effect(() => {
		if(!ban_list ||
			(ban_select_id > -1 && !ban_list.findItem((x) => x.id === ban_select_id)))
			ban_select_id = -1;
	});
	$effect(() => {
		if(ban_list && ban_select_id > -1){
			if(typeof state_bans.state.changed_bans[ban_select_id] === "undefined"){
				// Initialize ban change object
				let changed_bans = state_bans.state.changed_bans;
				changed_bans[ban_select_id] = {state: BanState.Unchanged};
				state_bans.set_all_states("changed_bans", changed_bans);
			}
		}
	});

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				apply_changes: () => {
					let changes = state_general.get_dict_of_changes();
					if(changes.avatar)
						changes.avatar = avatar_picker.getFile();
					else
						delete changes.avatar;
					Server.change(server_id, changes,
						() => state_general.apply_changes(),
						() => state_general.discard_changes());
				},
				discard_changes: () => state_general.discard_changes()
			},
			{ name: "Roles", render: roles, state: state_roles,
				apply_changes: () => {
					Role.change_list(server_id, state_roles.state.list,
								() => state_roles.apply_changes(),
								() => state_roles.discard_changes());
				},
				discard_changes: () => {
					role_list_selected_idx = -1;
					state_roles.discard_changes();
				}
			},
			{ name: "Bans", render: bans, state: state_bans,
				apply_changes: () => {
					let ban_changes = [];
					for(const id in state_bans.state.changed_bans){
						const change = state_bans.state.changed_bans[id];
						if(change.state !== BanState.Unchanged){
							change.id = id;
							ban_changes.push(change);
						}
					}

					let counter = ban_changes.length;
					// Changes should be discarded either way, because actual changes are already applied to Ban cache
					// thanks to socket events
					const _then = () => { if(--counter === 0) state_bans.discard_changes(); };
					const _catch = () => state_bans.discard_changes();

					for(const ch of ban_changes){
						if(ch.state === BanState.Changed)
							Ban.change(server_id, ch.id,
									ch.expires === "never" ? ch.expires : new Date(Date.now() + ch.expires),
									_then, _catch);
						else
							Ban.unban(server_id, ch.id, _then, _catch);
					}
				},
				discard_changes: () => state_bans.set_all_states("changed_bans", {})
			}
		];
	}
</script>

{#snippet general(params, close_settings)}
<Dialog bind:this={delete_confirm}
question="Delete the server?"
buttons={[{text: "Delete", action: () => {Server.delete(server_id, () => {}, () => {}); close_settings();}},
	  {text: "Cancel"}]}
>
This cannot be reversed.
</Dialog>
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker
		bind:this={avatar_picker}
		bind:display_url={state_general.state.avatar}
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

{#snippet render_ban(i, item)}
	<UserDisplay user={item} display_status={false}
			style={state_bans.state.changed_bans[item.id]?.state === BanState.Removed ? "text-decoration: line-through" : ""}
			selected={ban_select_id === item.id}
			onclick={() => ban_select_id = item.id}
	/>
{/snippet}

{#snippet bans(params, close_settings)}
<Group name="Ban list">
	<PaginatedList
		render_item={render_ban}
		load_items={(index, range) => Ban.get_range(server_id, index, range)}
		bind:this={ban_list}
	/>
</Group>
<Group>
	{#if ban_select_id > -1}
		<Button text={ban_button_text}
			onclick={() => {
				if(ban_select_change.state === BanState.Removed)
					ban_select_change.state = ban_select_change.expires ? BanState.Changed : BanState.Unchanged;
				else
					ban_select_change.state = BanState.Removed;
			}}
		/>
		{#if ban_select_change?.state === BanState.Changed || ban_select_change?.state === BanState.Unchanged}
			{#snippet render_ban_duration_select()}
				<Select options={[Util.TimeUnits.Minutes, Util.TimeUnits.Hours, Util.TimeUnits.Days]} option_labels={["min", "hr", "days"]}
					bind:value={ban_duration_units}
					--margin-bottom="0px" --width="96px"/>
			{/snippet}
			<Textbox label_text="Ban duration" error=""
				render_after={render_ban_duration_select} --width="128px"
				bind:value={() => {
							let expires;
							if(ban_select_change.state === BanState.Unchanged){
								const ban = ban_list.findItem((x) => x.id === ban_select_id);
								if(!ban)
									return "";
								expires = ban.expires;
								if(expires === "never")
									return "";
								expires = new Date(expires) - Date.now();
							}
							else{ // BanState.Changed
								expires = ban_select_change.expires;
								if(expires === "never")
									return "";
							}

							return Math.floor(expires / Util.time_unit_mul(ban_duration_units));
						},
					    (x) => {
							const expires = x === "" ? "never"
									: parseInt(x, 10) * Util.time_unit_mul(ban_duration_units);

							if(ban_select_change.state === BanState.Unchanged)
								ban_select_change.state = BanState.Changed;
							ban_select_change.expires = expires;
						}
					}
			/>
		{/if}
	{/if}
</Group>
{/snippet}
