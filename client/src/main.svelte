<script>
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	import MainSocket from '$lib/socket/main.js';

	import TabbedSettings from '$lib/control/tabbed_settings.svelte';
	let settings_params = $state({});
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

	import PaginatedList from '$lib/display/paginated_list.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import UserProfileDisplay from '$lib/display/user_profile.svelte';
	import MessageDisplay from '$lib/display/message.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenu from '$lib/control/context_menu.svelte';

	let { setPage } = $props();

	const setError = (err) => {
		if(err.reason)
			window.alert(err.type + ": " + err.reason);
		else
			window.alert(err.status + " " + err.data);
	};


	// Backend data
	let user_self = User.get(-1, setError);	
	let servers = Server.get_list(setError);
	let channels = $state([]);
	let roles = $state({});

	const getUserRoles = (user) => {
		if(!user || !user.roles)
			return [];

		let user_roles = [];
		for(const id of servers[sel.server].roles){
			const role_id = user.roles.find((x) => x === id);
			if(typeof role_id !== "undefined")
				user_roles.push(roles[role_id]);
		}
		return user_roles;
	};

	// Sockets
	let socket_main = new MainSocket(setError, setError);

	// UI state
	let wnd_width = $state(), wnd_height = $state();

	let sel = $state({
		server: -1, channel: -1,
		message: -1, message_edit: -1,
		user: {
			id: -1, message_id: -1
		}
	});

	let sel_user_pos = $derived.by(() => {
		wnd_width; wnd_height;
		message_list_scroll_top; server_user_list_scroll_top;

		if(sel.user.id < 0)
			return [[0, 0], [0, 0]];
		let el = document.getElementById(sel.user.message_id > -1
						? "message_display_" + sel.user.message_id
						: "user_display_" + sel.user.id);
		let brect = el.getBoundingClientRect();
		if(sel.user.message_id > -1)
			return [[brect.left, brect.top], [0, 0]];
		return [[brect.left, brect.top], [-100, 0]];
	});

	let message_text = $state("");
	let message_status = $state();
	let prev_message_text = "";
	let message_list = $state(), server_user_list = $state();
	let message_list_scroll_top = $state(0), server_user_list_scroll_top = $state(0);

	const is_message_fake = (msg_id) => {
		return pg_messages[msg_id].status === Message.Status.Sending
			|| typeof pg_messages[msg_id].status === "string";
	};

	const action_sets = {
		"message": [{text: "Edit", icon: "edit.svg", func: () => {
				sel.message_edit = sel.message;
				prev_message_text = message_text;
				message_text = message_list.getItem(sel.message).text;
			    }},
			    {text: "Delete", icon: "delete.svg", func: () => {
				let msg = message_list.getItem(sel.message);
				msg.status = Message.Status.Deleting;
				Message.delete(sel.server, sel.channel, msg.id,
						() => {}, setError);
			    }}],

		"server": [{text: "Settings", icon: "settings.svg", func: () => {
				sel.settings_tabs = settings_server.tabs();
			    }}],

		"channel": [{text: "Settings", icon: "settings.svg", func: () => {
				sel.settings_tabs = settings_channel.tabs();
			    }},
			    {text: "Delete", icon: "delete.svg", func: () => {
				Channel.delete(sel.server, settings_params.channel_id,
						() => {}, setError);
			    }}],
	};

	let ctx_menu_params = $state({
		visible: false,
		pos: [1000, 0],
		actions: []
	});
	const showCtxMenu = (pos, action_set, i) => {
		sel.message = i;
		ctx_menu_params.pos = pos;
		ctx_menu_params.actions = action_sets[action_set];
		ctx_menu_params.visible = true;
	};
	const hideCtxMenu = () => {
		sel.message = -1;
		ctx_menu_params.visible = false;
	};

	const closeSettings = () => {
		sel.settings_tabs = undefined;
	}

	// Events
	const showServer = (id) => {
		sel.server = id;
		sel.channel = -1;
		settings_params = {};
		channels = Channel.get_list(id, setError);
	};

	const showChannel = (id) => {
		showUser(-1, -1);
		sel.channel = id;
		settings_params = {};
	};

	let profile_display_user = $state();
	const showUser = (id, message_id) => {
		sel.user.id = id;
		sel.user.message_id = message_id;
		if(id > -1)
			profile_display_user = User.get_server(sel.server, sel.user.id);
		else
			profile_display_user = undefined;
	};

	const sendMessage = (text) => {
		message_status = "Sending...";
		Message.send(sel.server, sel.channel, text,
				() => {
					message_text = "";
					message_status = undefined;
				}, setError);
	};
	const editMessage = (text) => {
		let msg = message_list.getItem(sel.message_edit);
		let prev_text = msg.text;
		msg.status = Message.Status.Editing;
		msg.text = text;
		sel.message_edit = -1;
		message_text = "";

		Message.edit(sel.server, sel.channel, msg.id, text,
				() => {}, setError);
	};

	const stopEditing = () => {
		sel.message_edit = -1;
		message_text = prev_message_text;
	};
</script>


<style>
@import "main.css";
</style>


<svelte:window bind:innerWidth={wnd_width} bind:innerHeight={wnd_height}/>


<SettingsUser bind:this={settings_user}/>
<SettingsServer bind:this={settings_server} server_id={settings_params.server_id}/>
<SettingsChannel bind:this={settings_channel} server_id={sel.server} channel_id={settings_params.channel_id}/>

<CreateServer bind:this={create_server}/>
<CreateChannel bind:this={create_channel} server_id={sel.server}/>

{#if sel.settings_tabs}

<div style="padding: 16px; height: 100%">
	<TabbedSettings tabs={sel.settings_tabs} close_settings={closeSettings}/>
</div>
	
{:else}
	<div class="main">
		<div style="height: 100%; margin-left: 16px">
			<div style="display: flex; height: 90%">
				<div style="display: flex; flex-direction: column">
					<div class="panel sidebar_servers">
						{#if servers.loading}
						<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
						{:else}
						{#each servers as srv}
							<button class={"item hoverable sidebar_server" + (sel.server == srv.id ? " selected" : "")}
								onclick={() => showServer(srv.id)}
								oncontextmenu={(e) => {
									event.preventDefault();
									settings_params.server_id = srv.id;
									showCtxMenu([e.clientX, e.clientY], "server");
								}}
							>
							{#if srv.avatar === undefined}
							<div style="padding:4px;"><div class="sidebar_server_el">{srv.name}</div></div>
							{:else}
							<img class="sidebar_server_el" alt={srv.name} src={Server.get_avatar_path(srv)}/>
							{/if}
							</button>
						{/each}
						{/if}
					</div>
					<div class="panel sidebar_servers sidebar_server_actions">
						<button class={"item hoverable sidebar_server"}
							onclick={() => sel.settings_tabs = create_server.tabs()}
						>
						<img src="$lib/assets/icons/add.svg" alt="create server" class="filter_icon_main" style="width: 40px"/>
						</button>
					</div>
				</div>

				<div style="display: flex; flex-direction: column">
					<div class="panel sidebar_channels">
						{#if channels.loading}
							<div style="text-align: center; margin-top: 6px">
							<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
						</div>
						{:else}
						{#each channels as ch}
							<div>
								<button
								class={"item hoverable transparent_button sidebar_channel_el" + (sel.channel == ch.id ? " selected" : "")}
								onclick={() => showChannel(ch.id)}
								oncontextmenu={(e) => {
									event.preventDefault();
									settings_params.channel_id = ch.id;
									showCtxMenu([e.clientX, e.clientY], "channel");
								}}
								>
									{#if ch.type === 1}
									<img src="$lib/assets/icons/channel_vc.svg" alt="voice" class="filter_icon_main sidebar_channel_el_icon"/>
									{:else}
									<img src="$lib/assets/icons/channel_text.svg" alt="text" class="filter_icon_main sidebar_channel_el_icon"/>
									{/if}
									{ch.name}
								</button>
							</div>
						{/each}
						{/if}
					</div>
					{#if typeof channels.loading !== "undefined" && !channels.loading}
					<div class="panel sidebar_channels sidebar_channel_actions">
						<button
						class="item hoverable transparent_button sidebar_channel_el"
						style="border-style: solid none none none"
						onclick={() => sel.settings_tabs = create_channel.tabs()}
						>
							<img src="$lib/assets/icons/add.svg" alt="add channel" class="filter_icon_main sidebar_channel_el_icon"/>
							Add channel
						</button>
					</div>
					{/if}
				</div>
		</div>

		<div class="panel profile_panel">
			<button class="hoverable transparent_button"
			onclick={() => sel.settings_tabs = settings_user.tabs()}
			>
				<img src="$lib/assets/icons/settings.svg" alt="profile settings" class="filter_icon_main" style="width: 32px"/>
			</button>
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
				show_ctx_menu={(pos, action_set) => showCtxMenu(pos, action_set, i)}
				selected_user={item.id == sel.user.message_id && item.author_id == sel.user.id}
				onclick_user={() => showUser(item.author_id, item.id)}
				hide_profile={() => showUser(-1, -1)}
				/>
			{/snippet}
			<PaginatedList bind:this={message_list}
			bind:scrollTop={message_list_scroll_top}
			reversed={true}
			loading_text="Loading messages..." to_latest_text="To latest messages"
			render_item={render_message} item_dom_id_prefix="message_display_"
			load_items={(index, range) => Message.get_range(sel.server, sel.channel, index, range, setError)}
			augment_item={(msg) => {
							msg.author = User.get_server(sel.server, msg.author_id);
							msg.author_roles = Role.get_user_roles(msg.author, sel.server);
			}}
			}}/>
			<MessageInput
				bind:value={message_text} onsend={sel.message_edit > -1 ? editMessage : sendMessage}
				status={message_status}
				actions={sel.message_edit > -1 ? [{text: "Stop editing", func: stopEditing}] : []}/>
		{/if}
	</div>
	{#if ctx_menu_params.visible}
		<ContextMenu pos={ctx_menu_params.pos} hide_ctx_menu={hideCtxMenu}
			     actions={ctx_menu_params.actions}/>
	{/if}
	{#if sel.server > -1}
		<div class="panel sidebar_users">
			{#snippet render_user(i, user)}
				<UserDisplay
				user={user}
				user_roles={user.role_list}
				div_classes="sidebar_user_display"
				selected={sel.user.message_id == -1 && user.id == sel.user.id}
				onclick={() => showUser(user.id, -1)}
				hide_profile={() => showUser(-1, -1)}
				/>
			{/snippet}
			<PaginatedList bind:this={server_user_list}
			bind:scrollTop={server_user_list_scroll_top}
			render_item={render_user} item_dom_id_prefix="user_display_"
			load_items={(index, range) => User.get_server_range(sel.server, index, range, setError)}
			augment_item={(user) => {user.role_list = Role.get_user_roles(user, sel.server)}}
			to_latest_text="Up"/>
		</div>
	{/if}
</div>

{#if profile_display_user}
	<UserProfileDisplay
	user={profile_display_user} user_roles={Role.get_user_roles(profile_display_user, sel.server)}
	pos={sel_user_pos[0]} rel_off={sel_user_pos[1]}
	hide_profile={() => showUser(-1, -1)}
	/>
{/if}

{/if}
