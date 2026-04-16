<script>
	import {asset} from '$app/paths';

	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import Select from '$lib/control/select.svelte';
	import List from '$lib/control/list.svelte';
	import UserDisplay from '$lib/display/user.svelte';

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
	let wl_selected_idx = $state(-1);
	let wl_users = $state({});

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

	$effect(() => {
		console.log("STATE", $state.snapshot(state_access.state.wl_users));
	});

	export function tabs() {
		return [
			{name: "General", render: general, state: state_general},
			{name: "Access", render: access, state: state_access}
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
<Group name="Access">
	<List items={state_access.state.wl_roles.concat(state_access.state.wl_users)}
		render_item={render_wl}
		bind:selected_idx={wl_selected_idx}
	/>
</Group>
{/snippet}
