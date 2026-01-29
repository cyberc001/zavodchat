<script>
	import {tick} from 'svelte';

	import Util from '$lib/util.js';
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import ServerUser from '$lib/rest/server_user.js';
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

	import UserDisplay from '$lib/display/user.svelte';
	import UserProfileDisplay from '$lib/display/user_profile.svelte';
	import MessageDisplay from '$lib/display/message.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import VCPanel from '$lib/control/vc_panel.svelte';

	import ContextMenu from '$lib/control/context_menu.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	import Dialog from '$lib/control/dialog.svelte';
	import NotifDisplay from '$lib/display/notif.svelte';
	import Video from '$lib/display/video.svelte';

	let {setPage} = $props();

	const setError = (err) => {
		if(err.reason)
			window.alert(err.type + ": " + err.reason);
		else
			window.alert(err.status + " " + err.data);
	};

	// Backend data
	let user_self = User.get(-1);
	let servers = Server.get_list();

	let server = $state();
	let server_roles = $state([]);
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
								hideServer();
						} else if((name === "user_kicked" || name === "user_banned") && data.id === user_self.data.id){
							if(settings_params.server_id === data.id)
								closeSettings();
							if(data.server_id === sel.server)
								hideServer();
						} else if((name === "channel_deleted" || (name === "channel_edited" && data.type === Channel.Type.Voice)
								) && sel.channel === data.id){
							showServer(sel.server);
						}
					});

	// UI state
	let sel = $state({
		server: -1, channel: -1,
		message_edit: -1,
		user: {
			id: -1, message_id: -1
		},
		ctx: {
			message: -1, user_id: -1
		}
	});

	let message_text = $state("");
	let message_status = $state();
	let prev_message_text = "";
	let message_list = $state(), server_user_list = $state();

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
		sel.ctx.message = -1;
		sel.ctx.user_id = -1;
		ctx_menu_params.visible = false;
	};

	const closeSettings = () => {
		sel.settings_tabs = undefined;
		settings_params = {};
	}

	// Events
	const hideServer = (id) => {
		sel.server = -1;
		sel.channel = -1;

		server = {};
		server_roles = [];
		channels = undefined;
	};
	const showServer = (id) => {
		sel.server = id;
		sel.channel = -1;

		server = Server.get(id);
		server_roles = Role.get_list(id);
		channels = Channel.get_list(id);
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
			profile_display_params.anchor_side_x = message_id > -1 ? "left" : "right";
		} else {
			profile_display_params.user = null;
			profile_display_params.anchor = null;
		}
	};

	const sendMessage = (text) => {
		message_status = "Sending...";
		Message.send(sel.channel, text,
				() => {
					message_text = "";
					message_status = undefined;
				});
	};
	const editMessage = (text) => {
		let msg = message_list.getItem(sel.message_edit);
		let prev_text = msg.text;
		msg.status = Message.Status.Editing;
		msg.text = text;
		sel.message_edit = -1;
		message_text = "";

		Message.edit(msg.id, text,
				() => {}, () => {
						msg.status = Message.Status.None;
						msg.text = prev_text;
		});
	};

	const stopEditing = () => {
		sel.message_edit = -1;
		message_text = prev_message_text;
	};
</script>


<style>
@import "main.css";
</style>


{#snippet action_edit_message(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/edit.svg" text="Edit"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				sel.message_edit = sel.ctx.message;
				prev_message_text = message_text;
				message_text = message_list.getItem(sel.ctx.message).text;
		}}
	/>
{/snippet}
{#snippet action_delete_message(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/delete.svg" text="Delete"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				let msg = message_list.getItem(sel.ctx.message);
				msg.status = Message.Status.Deleting;
				Message.delete(msg.id,
						() => {}, () => msg.status = Message.Status.None);
		}}
	/>
{/snippet}

{#snippet action_settings_server(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/settings.svg" text="Settings"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				sel.settings_tabs = settings_server.tabs();
		}}
	/>
{/snippet}

{#snippet action_settings_channel(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/settings.svg" text="Settings"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				sel.settings_tabs = settings_channel.tabs();
		}}
	/>
{/snippet}
{#snippet action_delete_channel(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/delete.svg" text="Delete"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				Channel.delete(settings_params.channel_id,
						() => {}, () => {});
		}}
	/>
{/snippet}

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/kick.svg" text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				User.kick(sel.server, sel.ctx.user_id, () => {}, () => {});
		}}
	/>
{/snippet}
{#snippet action_ban_user(hide_ctx_menu)}
	<ContextMenuAction icon="src/lib/assets/icons/ban.svg" text="Ban"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				ban.user_id = sel.ctx.user_id;
				ban.duration = "";
				ban.duration_units = Util.TimeUnits.Minutes;
				ban.dialog.show();
		}}
	/>
{/snippet}

{#snippet user_volume()}
	<div style="padding: 4px">
		<Slider text="User volume" bind:value={() => Math.floor(socket_vc.audio[sel.ctx.user_id].volume * 100),
						(x) => socket_vc.audio[sel.ctx.user_id].set_volume(x / 100)}
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
						sel.ctx.user_id = vc_state.id;
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
				<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 24px"/>
			{:else}
				<div style="display: flex; align-items: center; margin-bottom: 6px">
					<img src={User.get_avatar_path(user_self.data)} style="width: 32px; height: 32px; margin-right: 8px" alt="avatar"/>
					{user_self.data.name}
					<button class="hoverable transparent_button"
						style="margin-left:auto"
						onclick={() => sel.settings_tabs = settings_user.tabs()}
					>
						<img src="$lib/assets/icons/settings.svg" alt="profile settings" class="filter_icon_main" style="width: 32px"/>
					</button>
				</div>
			{/if}
		</div>
	</div>
	<div class="panel sidebar_messages">
		{#if sel.channel > -1}
			{#snippet render_message(i, item)}
				<MessageDisplay id={item.id} text={item.text}
				author={item.author}
				author_roles={item.author_roles}
				time_sent={new Date(item.sent)} time_edited={new Date(item.edited)}
				status={item.status}
				show_ctx_menu={(anchor, e, for_message) => {
					sel.ctx.user_id = item.author.data.id;
					sel.ctx.message = i;
					showCtxMenu(anchor, e, for_message ? [action_edit_message, action_delete_message]
								: [action_kick_user, action_ban_user]);
				}}
				selected_user={item.id == sel.user.message_id && item.author_id == sel.user.id}
				onclick_user={() => showUser(item.author_id, item.id)}
				hide_profile={() => showUser(-1, -1)}
				/>
			{/snippet}
			<PaginatedList bind:this={message_list}
			reversed={true}
			loading_text="Loading messages..." to_latest_text="To latest messages"
			render_item={render_message}
			load_items={(index, range) => Message.get_range(sel.channel, index, range)}
			augment_item={(msg) => {
						msg.author = User.get_server(sel.server, msg.author_id);
						msg.author_roles = Role.get_user_roles(msg.author.data, server_roles.data);
			}}
			}}/>
			<MessageInput
				bind:value={message_text} onsend={sel.message_edit > -1 ? editMessage : sendMessage}
				status={message_status}
				actions={sel.message_edit > -1 ? [{text: "Stop editing", func: stopEditing}] : []}/>
		{/if}
	</div>
	{#if sel.server > -1}
		<div class="panel sidebar_users">
			{#snippet render_user(i, user)}
				<UserDisplay
				user={user} user_roles={user.role_list}
				selected={sel.user.message_id == -1 && user.id == sel.user.id}
				onclick={() => showUser(user.id, -1)}
				show_ctx_menu={(anchor, e) => {
					sel.ctx.user_id = user.id;
					showCtxMenu(anchor, e, [action_kick_user, action_ban_user]);
				}}
				/>
			{/snippet}
			<PaginatedList bind:this={server_user_list}
			render_item={render_user}
			load_items={(index, range) => User.get_server_range(sel.server, index, range)}
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
		assign_role={(role_id) => ServerUser.assign_role(sel.server, profile_display_params.user.data.id, role_id,
									() => {})}
		disallow_role={(role_id) => ServerUser.disallow_role(sel.server, profile_display_params.user.data.id, role_id,
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

{#if socket_vc && socket_vc.watched_video}
	<Video track={socket_vc.watched_video}
		close_video={() => socket_vc.unwatch_video()}
	/>
{/if}

<NotifDisplay/>

<Dialog bind:this={ban.dialog}
	question="Ban user?"
	buttons={[{text: ban.duration ? "Ban" : "Ban forever", disabled: ban.error,
			action: () => {
				Ban.ban(sel.server, ban.user_id, ban.expires, () => {}, () => {})
			}},
		  {text: "Cancel"}]}
>
	<DurationPicker label_text="Ban duration"
		bind:expires={ban.expires} bind:error={ban.error}
	/>
</Dialog>
