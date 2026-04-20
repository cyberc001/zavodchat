<script>
	import {asset} from '$app/paths';

	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import Button from '$lib/control/button.svelte';
	import Select from '$lib/control/select.svelte';
	import List from '$lib/control/list.svelte';
	import Toggle from '$lib/control/toggle.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import UserPicker from '$lib/control/user_picker.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	let { server_id, channel_id } = $props();

	let server = Server.get(server_id);
	let channel = Channel.get(channel_id);
	let server_roles = Role.get_list(server_id);
	let roles = $state({});
	$effect(() => {
		if(server_roles.loaded)
			for(const role of server_roles.data)
				roles[role.id] = role;
	});

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


	// Access
	let wl_picker = $state();
	let wl_users = $state({});
	let wl_add_id = $state();
	let wl_add_is_role = $state(false);

	class AccessTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!server_roles.loaded)
				return SettingsTabState.ChangesState.Loading;
		});

		apply_changes(){
			Channel.change(channel_id,
				super.get_dict_of_changes(),
				() => super.apply_changes(),
				() => super.discard_changes());
		}
	};
	let state_access = new AccessTabState({wl_users: [], wl_roles: []});
	$effect(() => {
		if(channel.loaded){
			if(channel.data.wl_users){
				for(const id of channel.data.wl_users)
					wl_users[id] = User.get_server(server_id, id);
				state_access.set_default_state("wl_users", channel.data.wl_users);
			}
			if(channel.data.wl_roles)
				state_access.set_default_state("wl_roles", channel.data.wl_roles);
		}
	});


	// Permissions
	let perms_role_picker = $state();
	let perms_role_add_id = $state();
	let perms_role_selected_idx = $state(-1);
	let perms_role_selected = $derived(state_permissions.state.roles[perms_role_selected_idx]);

	class PermissionsTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!server_roles.loaded)
				return SettingsTabState.ChangesState.Loading;
		});

		apply_changes(){
			this.execute_list_changes("roles",
				(role, _then, _catch) => Channel.change_role_perms(channel_id, role.id, role, _then, _catch),
				(role, _then, _catch) => Channel.change_role_perms(channel_id, role.id, role, _then, _catch),
				(role, _then, _catch) => Channel.delete_role_perms(channel_id, role.id, _then, _catch)
			);
		}
	};
	let state_permissions = new PermissionsTabState({roles: []});
	$effect(() => {
		if(channel.loaded && channel.data.roles)
			state_permissions.set_default_state("roles", channel.data.roles);
	});


	export function tabs() {
		return [
			{name: "General", render: general, state: state_general},
			{name: "Access", render: access, state: state_access},
			{name: "Permissions", render: permissions, state: state_permissions}
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


{#snippet render_wl(i, id)}
<div style="display: flex">
{#if i < state_access.state.wl_roles.length}
	<div style="font-size: 18px; {Role.get_color_style(roles[id])}">
		{roles[id].name}
	</div>	
{:else}
	<UserDisplay user={wl_users[id]} server={server}
	display_status={false}/>
{/if}

<div style="margin-left: auto">
	<button class="transparent_button hoverable"
	onclick={() => i < state_access.state.wl_roles.length ?
			state_access.state.wl_roles.splice(i, 1) :
			state_access.state.wl_users.splice(i - state_access.state.wl_roles.length, 1)
		}>
		<img src={asset("icons/close.svg")} alt="remove from whitelist" class="filter_icon_main" style="width: 32px"/>
	</button>
</div>
</div>
{/snippet}

{#snippet access()}
<Group name="Whitelist">
	<List items={state_access.state.wl_roles.concat(state_access.state.wl_users)}
		selectable={false}
		render_item={render_wl}
	/>
	<div style="display: flex; margin-top: 16px">
		<UserPicker server={server}
		prepended_roles={server_roles.data.filter((x) => state_access.state.wl_roles.findIndex((y) => y === x.id) === -1)}
		bind:this={wl_picker} bind:value={wl_add_id} bind:value_is_role={wl_add_is_role}
		user_picked={(id) => {
			wl_users[id] = User.get_server(server_id, id);
		}}
		--width="min(400px, 50vw)"
		--margin-bottom="0"
		--margin-right="6px"
		/>
		<Button text="Add"
		disabled={typeof(wl_add_id) === "undefined" || 
			  (!wl_add_is_role && state_access.state.wl_users.findIndex((x) => x === wl_add_id) !== -1)}
		onclick={() => {
			if(typeof(wl_add_id) === "undefined")
				return;

			if(wl_add_is_role)
				state_access.state.wl_roles.push(wl_add_id);
			else
				state_access.state.wl_users.push(wl_add_id);

			wl_picker.reset();
		}}
		--margin-bottom="0"
		/>

		<div style="margin-left: auto">
			<Button text="Clear whitelist"
			disabled={state_access.state.wl_users.length === 0 && state_access.state.wl_roles.length === 0}
			onclick={() => {
				state_access.state.wl_users = [];
				state_access.state.wl_roles = [];
			}}
			--margin-bottom="0"
			/>
		</div>
	</div>
</Group>
{/snippet}


{#snippet render_role(i, role)}
<div style="display:flex">
<div style="font-size: 18px; {Role.get_color_style(roles[role.id])}">
	{roles[role.id].name}
</div>

<div style="margin-left: auto">
	<button class="transparent_button hoverable"
	onclick={() => state_permissions.state.roles.splice(i, 1)}>
		<img src={asset("icons/close.svg")} alt="remove role permissions" class="filter_icon_main" style="width: 32px"/>
	</button>
</div>
</div>
{/snippet}

{#snippet permissions()}
<Group name="Role list">
	<List items={state_permissions.state.roles}
		render_item={render_role}
		bind:selected_idx={perms_role_selected_idx} 
	/>
	<div style="display: flex; margin-top: 16px">
		<UserPicker server={server} load_users={false}
		prepended_roles={server_roles.data.filter((x) => state_permissions.state.roles.findIndex((y) => y.id === x.id) === -1)}
		bind:this={perms_role_picker} bind:value={perms_role_add_id}
		--width="min(400px, 50vw)"
		--margin-bottom="0"
		--margin-right="6px"
		/>
		<Button text="Add"
		onclick={() => {
			if(typeof(perms_role_add_id) === "undefined")
				return;
			state_permissions.state.roles.push({id: perms_role_add_id, perms1: 0});
			perms_role_picker.reset();
		}}
		--margin-bottom="0"
		/>
	</div>
</Group>
<Group name="Role permissions">
{#if perms_role_selected}
	<Toggle label_text="Send messages" states="off_default_on"
		bind:value={() => Role.perm_toggle_get(perms_role_selected, 1, 0),
			    (x) => Role.perm_toggle_set(x, perms_role_selected, 1, 0)}
	/>
{/if}
</Group>
{/snippet}
