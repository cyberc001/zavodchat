<script>
	import {tick} from 'svelte';
	import {asset} from '$app/paths';

	import Util from '$lib/util.js';

	import Params from '$lib/rest/params.js';
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import Ban from '$lib/rest/ban.js';

	import MainSocket from '$lib/socket/main.js';
	import VCSocket from '$lib/socket/vc.svelte.js';

	import TabbedSettings from '$lib/control/tabbed_settings.svelte';
	let settings_params = $state({});
	const setSettingsParams = (params) => {
		settings_params = {};
		tick().then(() => {
			settings_params = params;
		});
	}

	import SettingsUser from '$lib/settings/user.svelte';
	let settings_user = $state();
	import SettingsServer from '$lib/settings/server.svelte';
	let settings_server = $state();
	import SettingsChannel from '$lib/settings/channel.svelte';
	let settings_channel = $state();

	import CreateServer from '$lib/settings/create_server.svelte';
	let create_server = $state();
	import CreateChannel from '$lib/settings/create_channel.svelte';
	let create_channel = $state();

	import DurationPicker from '$lib/control/duration_picker.svelte';
	import Slider from '$lib/control/slider.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';

	import SidebarServer from '$lib/display/sidebar/server.svelte';
	import SidebarChannel from '$lib/display/sidebar/channel.svelte';
	import SidebarMessage from '$lib/display/sidebar/message.svelte';
	import SidebarFriends from '$lib/display/sidebar/friends.svelte';

	import UserDisplay from '$lib/display/user.svelte';
	import UserProfileDisplay from '$lib/display/user_profile.svelte';
	import VCPanel from '$lib/control/vc_panel.svelte';

	import ContextMenu from '$lib/control/context_menu.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	import Dialog from '$lib/control/dialog.svelte';
	import NotifDisplay from '$lib/display/notif.svelte';

	let {setPage} = $props();

	const setError = (err) => {
		if(err.reason)
			window.alert(err.type + ": " + err.reason);
		else
			window.alert(err.status + " " + err.data);
	};

	// Backend data
	Params.load();
	let user_self = User.get(-1);
	let servers = Server.get_list();

	let server = $state();
	let server_roles = $state();
	let channels = $state();

	// Sockets
	let socket_vc = $state();
	let socket_main = new MainSocket(setError, setError,
					(name, data) => {
						socket_vc?.on_main_message(name, data);

						if(name === "server_deleted"){
							if(settings_params.server_id === data.id)
								closeSettings();
							if(data.id === sel.server)
								showServer(-1);
						} else if((name === "user_kicked" || name === "user_banned") && data.id === user_self.data.id){
							if(settings_params.server_id === data.id)
								closeSettings();
							if(data.server_id === sel.server)
								showServer(-1);
						} else if((name === "channel_deleted" || (name === "channel_edited" && data.type === Channel.Type.Voice)
								) && sel.channel === data.id){
							showServer(sel.server);
						}
					});

	// UI state
	let sel = $state({
		server: -1, channel: -1,
		user: {
			id: -1, message_id: -1
		},
		ctx_user_id: -1
	});

	let server_user_list = $state();

	let ban = $state({
		user_id: -1,
		dialog: null,
		expires: "never", error: ""
	});

	let ctx_menu_params = $state({
		anchor: null,
		visible: false,
		off: [0, 0],
		items: []
	});
	const showCtxMenu = (anchor, e, items) => {
		ctx_menu_params.anchor = anchor;
		const rect = anchor.getBoundingClientRect();
		ctx_menu_params.off = [e.clientX - rect.left, e.clientY - rect.top];

		ctx_menu_params.items = items;
		ctx_menu_params.visible = true;
	};
	const hideCtxMenu = () => {
		ctx_menu_params.visible = false;
		showUser(-1, -1);
	};

	const closeSettings = () => {
		sel.settings_tabs = undefined;
		settings_params = {};
	}

	// Events
	const showServer = (id) => {
		sel.server = id;
		sel.channel = -1;

		if(id > -1){
			server_user_list?.reset();
			server = Server.get(id);
			server_roles = Role.get_list(id);
			channels = Channel.get_list(id);
		} else {
			server = {};
			channels = undefined;
		}
	};

	const showChannel = (id, i) => {
		if(channels.data[i].type === Channel.Type.Voice){
			let old_socket_vc = socket_vc;
			socket_vc = new VCSocket(user_self.data.id, id, (close) => {
				if(close.reason === "User is already connected to this channel")
					socket_vc = old_socket_vc;
			});
		} else {
			showUser(-1, -1);
			sel.channel = id;
		}
	};


	let profile_display_params = $state({
		user: null, anchor: null, anchor_side_x: "left"
	});

	const showUser = (id, message_id) => {
		sel.user.id = id;
		sel.user.message_id = message_id;
		if(id > -1){
			profile_display_params.user = User.get_server(sel.server, sel.user.id);
			profile_display_params.anchor = document.getElementById(message_id > -1
						? "user_display_" + id + "_" + message_id
						: "user_display_" + id);
			if(!profile_display_params.anchor){
				profile_display_params.user = null;
				profile_display_params.anchor = null;
			} else
				profile_display_params.anchor_side_x = message_id > -1 ? "left" : "right";
		} else {
			profile_display_params.user = null;
			profile_display_params.anchor = null;
		}
	};

	const showBan = (id) => {
		ban.user_id = id;
		ban.expires = "never";
		ban.duration_units = Util.TimeUnits.Minutes;
		ban.dialog.show();
	};
</script>


<style>
@import "main.css";
</style>


{#snippet action_settings_server(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/settings.svg")} text="Settings"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				sel.settings_tabs = settings_server.tabs();
		}}
	/>
{/snippet}

{#snippet action_settings_channel(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/settings.svg")} text="Settings"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				sel.settings_tabs = settings_channel.tabs();
		}}
	/>
{/snippet}
{#snippet action_delete_channel(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/delete.svg")} text="Delete"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				Channel.delete(settings_params.channel_id,
						() => {}, () => {});
		}}
	/>
{/snippet}

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				User.kick(sel.server, sel.ctx_user_id, () => {}, () => {});
		}}
	/>
{/snippet}
{#snippet action_ban_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/ban.svg")} text="Ban"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => showBan(sel.ctx_user_id)}
	/>
{/snippet}

{#snippet user_volume()}
	<div style="padding: 4px">
		<Slider text="User volume" bind:value={() => Math.floor(socket_vc.audio[sel.ctx_user_id].volume * 100),
						(x) => socket_vc.audio[sel.ctx_user_id].set_volume(x / 100)}
			display_value={(value) => value + "%"}/>
	</div>
{/snippet}


<SettingsUser bind:this={settings_user}/>
{#if typeof settings_params.server_id !== "undefined"}
	<SettingsServer bind:this={settings_server} server_id={settings_params.server_id}/>
{/if}
{#if typeof settings_params.channel_id !== "undefined"}
	<SettingsChannel bind:this={settings_channel} server_id={sel.server} channel_id={settings_params.channel_id}/>
{/if}

<CreateServer bind:this={create_server}/>
<CreateChannel bind:this={create_channel} server_id={sel.server}/>

{#if sel.settings_tabs}
	<div style="padding: 16px; box-sizing: border-box; height: 100%">
		<TabbedSettings tabs={sel.settings_tabs} close_settings={closeSettings}/>
	</div>
{:else}
	<div class="main">
		<div style="height: 100%; width: 322px; margin-left: 16px; display: flex; flex-direction: column">
			<div style="display: flex; height: 100%">
				<SidebarServer servers={servers} selected_serever={sel.server}
					show_server={(server) => showServer(server.id)}
					show_friends={() => showServer(-1)}
					ctx_server={(self, e, server) => {
						setSettingsParams({server_id: server.id});
						showCtxMenu(self, e, [action_settings_server]);
					}}
					create_server={() => sel.settings_tabs = create_server.tabs()}
				/>

				<SidebarChannel server={server} channels={channels} selected_channel={sel.channel}
					socket_vc={socket_vc}
					show_channel={showChannel}
					ctx_channel={(self, e, channel) => {
						setSettingsParams({channel_id: channel.id});
						showCtxMenu(self, e, [action_settings_channel, action_delete_channel]);
					}}
					ctx_vc_user={(self, e, vc_state) => {
						if(vc_state.id === user_self.data.id)
							return;
						sel.ctx_user_id = vc_state.id;
						showCtxMenu(self, e, [user_volume]);
					}}
					create_channel={() => {
						sel.settings_tabs = create_channel.tabs();
					}}
				/>
		</div>

		{#if socket_vc}
			<VCPanel socket_vc={socket_vc} end_call={() => {
				socket_vc.end_call();
				socket_vc = undefined;
			}}/>
		{/if}
		<div class="panel profile_panel">
			{#if !user_self.loaded}
				<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="width: 24px"/>
			{:else}
				<div style="display: flex; align-items: center; margin-bottom: 6px">
					<img src={User.get_avatar_path(user_self.data)} style="width: 32px; height: 32px; margin-right: 8px" alt="avatar"/>
					{user_self.data.name}
					<button class="hoverable transparent_button"
						style="margin-left:auto"
						onclick={() => sel.settings_tabs = settings_user.tabs()}
					>
						<img src={asset("icons/settings.svg")} alt="profile settings" class="filter_icon_main" style="width: 32px"/>
					</button>
				</div>
			{/if}
		</div>
	</div>

	{#if sel.server > -1}
		<SidebarMessage server_id={sel.server} channel_id={sel.channel}
			sel_message_id={sel.user.message_id} sel_user_id={sel.user.id}
			show_ctx_menu={showCtxMenu} show_user={showUser}
			show_ban={showBan}
		/>
	{:else}
		<SidebarFriends />
	{/if}

	{#if sel.server > -1}
		<div class="panel sidebar_users">
			{#snippet render_user(i, user)}
				<UserDisplay
				user={user} user_roles={user.role_list}
				selected={sel.user.message_id == -1 && user.id == sel.user.id}
				onclick={() => showUser(user.id, -1)}
				show_ctx_menu={(anchor, e) => {
					sel.ctx_user_id = user.id;
					showCtxMenu(anchor, e, [action_kick_user, action_ban_user]);
				}}
				/>
			{/snippet}
			<PaginatedList bind:this={server_user_list}
			render_item={render_user}
			load_items={(start_id, range, asc) => User.get_server_range(sel.server, start_id, range, asc)}
			augment_item={(user) => {user.role_list = Role.get_user_roles(user, server_roles.data)}}
			to_latest_text="Up"/>
		</div>
	{/if}
</div>

{#if profile_display_params.user}
	<UserProfileDisplay
		anchor={profile_display_params.anchor} anchor_side_x={profile_display_params.anchor_side_x}
		user={profile_display_params.user.data} server_roles={server_roles.data}
		hide_profile={() => showUser(-1, -1)}
		assign_role={(role_id) => User.assign_role(sel.server, profile_display_params.user.data.id, role_id,
									() => {})}
		disallow_role={(role_id) => User.disallow_role(sel.server, profile_display_params.user.data.id, role_id,
									() => {})}
	/>
{/if}

{#if ctx_menu_params.visible}
	<ContextMenu
		anchor={ctx_menu_params.anchor} off={ctx_menu_params.off}
		hide_ctx_menu={hideCtxMenu}
		items={ctx_menu_params.items}/>
{/if}

{/if}

<NotifDisplay/>

<Dialog bind:this={ban.dialog}
	question="Ban user?"
	buttons={[{text: ban.expires === "never" ? "Ban forever" : "Ban", disabled: ban.error,
			action: () => {
				Ban.ban(sel.server, ban.user_id, ban.expires, () => {}, () => {})
			}},
		  {text: "Cancel"}]}
>
	<DurationPicker label_text="Ban duration"
		bind:expires={ban.expires} bind:error={ban.error}
	/>
</Dialog>
