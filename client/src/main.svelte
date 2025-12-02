<script>
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import UserDisplay from '$lib/display/user.svelte';
	import UserProfileDisplay from '$lib/display/user_profile.svelte';
	import MessageDisplay from '$lib/display/message.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenu from '$lib/control/context_menu.svelte';

	import Rest from '$lib/rest.js';
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.js';
	import Role from '$lib/rest/role.js';

	let { setPage } = $props();

	const setError = (err) => {
		window.alert(err.status + " " + err.data);
	};

	// Backend data
	let user_self = $state();
	// global
	let servers = $state("loading");
	let channels = $state({});
	let roles = $state({});
	// local
	let server_message_users = $state({});
	// paginated
	let pg_messages = $state([]);

	let profile_display_user = $derived.by(() => {
		return undefined;

		if(sel.user.id < 0)
			return undefined;
		if(sel.user.message_id > -1)
			return server_message_users[sel.user.id];
		const i = pg_server_users.findIndex((user) => user.id == sel.user.id);
		if(i > -1)
			return pg_server_users[i];
	});

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


	// DEBUG
	$effect(() => {
		console.log("server_message_users", $state.snapshot(server_message_users));
	});

	const ensureUser = (id) => {
		if(typeof server_message_users[id] === "undefined"){
			server_message_users[id] = User.get_server(sel.server, id, setError);

			//server_message_users[id] = User.dummy();

			//const su = pg_server_users.find((user) => user.id == id);
			//if(typeof su === "undefined"){
				//User.get_server(sel.server, id, (data) => {
				//	server_message_users[id] = data;
				//}, setError);
			//} else
			//	server_message_users[id] = su;
		}
	};

	// UI state
	let wnd_width = $state(), wnd_height = $state();

	let sel = $state({
		server: -1, channel: -1, message: -1, message_edit: -1,
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
	let prev_message_text = "";
	let message_list = $state(), server_user_list = $state();
	let message_list_scroll_top = $state(0), server_user_list_scroll_top = $state(0);

	const is_message_fake = (msg_id) => {
		return pg_messages[msg_id].status === Message.Status.Sending
			|| typeof pg_messages[msg_id].status === "string";
	};
	const action_sets = {
		"message": [{text: "Edit", icon: "edit.svg", func: () => {
				if(is_message_fake(sel.message))
					return;
				sel.message_edit = sel.message;
				prev_message_text = message_text;
				message_text = pg_messages[sel.message].text;
			    }},
			    {text: "Delete", icon: "delete.svg", func: () => {
				if(is_message_fake(sel.message))
					return;
				pg_messages[sel.message].status = Message.Status.Deleting;
				let chan_id = sel.channel, msg_i = sel.message; // "capture" ids
				Message.delete(sel.server, chan_id, pg_messages[msg_i].id,
						() => message_list.rerender(), setError);
			    }}]
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

	// Events
	const showServer = (id) => {
		pg_messages = [];
		server_message_users = {};
		sel.server = id;
		sel.channel = -1;

		server_message_users[user_self.id] = User.get_server(id, user_self.id, setError);
		//User.get_server(id, user_self.id, (data) => { server_message_users[-1] = data; }, setError);

		channels = Channel.get_list(id, setError);
		/*if(servers[id].channels !== "loading" && typeof servers[id].channels === "undefined"){
			servers[id].channels = "loading";
			channels = Channel.get_list(id, setError);

			Channel.get_list(id,
					(list) => {
						servers[id].channels = [];
						for(let ch of list){
							servers[id].channels.push(ch.id);
							channels[ch.id] = ch;
						}
					},
					setError);
		}*/
		if(servers[id].roles !== "loading" && typeof servers[id].roles === "undefined"){
			servers[id].roles = "loading";
			Role.get_list(id,
					(list) => {
						servers[id].roles = [];
						for(let rol of list){
							servers[id].roles.push(rol.id);
							roles[rol.id] = rol;
						}
					},
					setError);
		}
	};

	const showChannel = (id) => {
		showUser(-1, -1);
		Rest.cancel_request("Message.get_range");
		sel.channel = id;
		if(message_list)
			message_list.rerender(undefined, false);
	};

	const showUser = (id, message_id) => {
		sel.user.id = id;
		sel.user.message_id = message_id;
	};

	const sendMessage = (text) => {
		let chan_id = sel.channel;
		let pre_id = pg_messages.length > 0 ? pg_messages[pg_messages.length - 1].id + 1000000 : 0;
		let now = new Date(Date.now());
		pg_messages.unshift({
			author_id: user_self.id,
			id: pre_id,
			sent: now.toISOString(), edited: now.toISOString(),
			text: text,
			status: Message.Status.Sending
		});

		Message.send(sel.server, sel.channel, text,
				(new_msg_id) => message_list.rerender(),
				(err) => {
					pg_messages[0].status = err.data;
					//setError(err);
				}
		);
	};
	const editMessage = (text) => {
		let chan_id = sel.channel, msg_i = sel.message_edit;
		let prev_text = pg_messages[msg_i].text;
		pg_messages[msg_i].status = Message.Status.Editing;
		pg_messages[msg_i].text = text;
		sel.message_edit = -1;
		
		Message.edit(sel.server, chan_id, pg_messages[msg_i].id, text,
				() => {
					pg_messages[msg_i].status = Message.Status.None;
				}
				, (err) => {
					pg_messages[msg_i].text = prev_text;
					setError(err);
				});
	};

	const stopEditing = () => {
		sel.message_edit = -1;
		message_text = prev_message_text;
	};

	// Initialization
	User.get(-1, (data) => { user_self = data; }, setError);
	Server.get_list((list) => {
					let data_servers = {};
					for(let srv of list)
						data_servers[srv.id] = srv;
					servers = data_servers;
				}
				, setError);
</script>


<style>
	@import "main.css";
</style>


<svelte:window bind:innerWidth={wnd_width} bind:innerHeight={wnd_height}/>
<div class="main">
	<div class="panel sidebar_servers">
		{#if servers === "loading"}
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
		{:else if typeof servers !== "undefined"}
			{#each Object.values(servers) as srv}
				<button class={"item hoverable sidebar_server" + (sel.server == srv.id ? " selected" : "")} onclick={() => showServer(srv.id)}>
				{#if srv.avatar === undefined}
					<div style="padding:4px;"><div class="sidebar_server_el">{srv.name}</div></div>
					{:else}
					<img class="sidebar_server_el" alt={srv.name} src={Server.get_avatar_path(srv)}/>
				{/if}
				</button>
			{/each}
		{/if}
	</div>
	<div class="panel sidebar_channels">
		{#each channels as ch}
			<div>
				<button class={"item hoverable sidebar_channel_el" + (sel.channel == ch.idd ? " selected" : "")} onclick={() => showChannel(ch.id)}>
					{#if ch.type === 1}
						<img src="$lib/assets/icons/channel_vc.svg" alt="voice" class="filter_icon_main sidebar_channel_el_icon"/>
					{:else}
						<img src="$lib/assets/icons/channel_text.svg" alt="text" class="filter_icon_main sidebar_channel_el_icon"/>
					{/if}
					{ch.name}
				</button>
			</div>
		{/each}
	</div>
	<div class="panel sidebar_messages">
		{#if sel.channel > -1}
			{#snippet render_message(i, item)}
				<MessageDisplay id={item.id} text={item.text}
				author={server_message_users[item.author_id]}
				author_roles={getUserRoles(server_message_users[item.author_id])}
				time_sent={new Date(item.sent)} time_edited={new Date(item.edited)}
				status={item.status}
				show_ctx_menu={(pos, action_set) => showCtxMenu(pos, action_set, i)}
				selected_user={item.id == sel.user.message_id && item.author_id == sel.user.id}
				onclick_user={() => showUser(item.author_id, item.id)}
				hide_profile={() => showUser(-1, -1)}
				/>
			{/snippet}
			<PaginatedList bind:items={pg_messages} bind:this={message_list}
			bind:scrollTop={message_list_scroll_top}
			reversed={true}
			loading_text="Loading messages..." to_latest_text="To latest messages"
			render_item={render_message} item_dom_id_prefix="message_display_"
			load_items={(index, count, _then) => {
				Message.get_range(sel.server, sel.channel, index, count,
					(list) => {
						for(let msg of list)
							ensureUser(msg.author_id);
						_then(list);
					}
					, setError);
			}}/>
			<MessageInput
				bind:value={message_text} onsend={sel.message_edit > -1 ? editMessage : sendMessage}
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
				roles={getUserRoles(user)}
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
			to_latest_text="Up"/>
		</div>
	{/if}
</div>

{#if profile_display_user}
	<UserProfileDisplay
	user={profile_display_user}
	roles={getUserRoles(profile_display_user)}
	pos={sel_user_pos[0]} rel_off={sel_user_pos[1]}
	hide_profile={() => showUser(-1, -1)}
	/>
{/if}
