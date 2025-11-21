<script>
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import MessageDisplay from '$lib/display/message.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenu from '$lib/control/context_menu.svelte';

	import Rest from '$lib/rest.js';
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.js';

	let { setPage } = $props();

	const setError = (err) => {
		window.alert(err.status + " " + err.data);
	};

	// Backend data
	let servers = $state("loading");
	let channels = $state({});
	let messages = $state([]);
	let users = $state({});

	const ensureUser = (id) => {
		if(typeof users[id] === "undefined"){
			// TODO prevent double-loading
			User.get(id, (data) => { users[id] = data }, setError);
		}
	};

	// UI state
	let sel = $state({
		server: -1, channel: -1, message: -1, message_edit: -1
	});
	let message_text = $state("");
	let message_list;

	const is_message_fake = (msg_id) => {
		return messages[msg_id].status === Message.Status.Sending
			|| typeof messages[msg_id].status === "string";
	};
	const action_sets = {
		"message": [{text: "Edit", icon: "edit.svg", func: () => {
				if(is_message_fake(sel.message))
					return;
				sel.message_edit = sel.message;
				message_text = messages[sel.message].text;
			    }},
			    {text: "Delete", icon: "delete.svg", func: () => {
				if(is_message_fake(sel.message))
					return;
				messages[sel.message].status = Message.Status.Deleting;
				let chan_id = sel.channel, msg_i = sel.message; // "capture" ids
				Message.delete(sel.server, chan_id, messages[msg_i].id,
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
		sel.server = id;
		sel.channel = -1;
		if(servers[id].channels !== "loading" && typeof servers[id].channels === "undefined"){
			servers[id].channels = "loading";
			Channel.get_list(id,
					(list) => {
						servers[id].channels = [];
						for(let ch of list){
							servers[id].channels.push(ch.id);
							channels[ch.id] = ch;
						}
					},
					setError);
		}
	};
	const showChannel = (id) => {
		sel.channel = id;
	};

	const sendMessage = (text) => {
		let chan_id = sel.channel;
		let pre_id = messages.length > 0 ? messages[messages.length - 1].id + 1000000 : 0;
		let now = new Date(Date.now());
		messages.unshift({
			author_id: users[-1].id,
			id: pre_id,
			sent: now.toISOString(), edited: now.toISOString(),
			text: text,
			status: Message.Status.Sending
		});

		Message.send(sel.server, sel.channel, text,
				(new_msg_id) => message_list.rerender(),
				(err) => {
					messages[0].status = err.data;
					//setError(err);
				}
		);
	};
	const editMessage = (text) => {
		let chan_id = sel.channel, msg_i = sel.message_edit;
		let prev_text = messages[msg_i].text;
		messages[msg_i].status = Message.Status.Editing;
		messages[msg_i].text = text;
		sel.message_edit = -1;
		
		Message.edit(sel.server, chan_id, messages[msg_i].id, text,
				() => {
					messages[msg_i].status = Message.Status.None;
				}
				, (err) => {
					messages[msg_i].text = prev_text;
					setError(err);
				});
	};

	const stopEditing = () => {
		sel.message_edit = -1;
		message_text = "";
	};

	// Initialization
	User.get(-1, (data) => { users[-1] = data; }, setError);
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
	<div class="panel sidebar_channel">
		{#if sel.server > -1}
			{#if servers[sel.server].channels === "loading"}
				<div style="text-align: center; margin-top: 6px">
					<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
				</div>
			{:else}
				{#each servers[sel.server].channels as i}
					<div>
						<button class={"item hoverable sidebar_channel_el" + (sel.channel == i ? " selected" : "")} onclick={() => showChannel(i)}>
							{#if channels[i].type === 1}
								<img src="$lib/assets/icons/channel_vc.svg" alt="voice" class="filter_icon_main sidebar_channel_el_icon"/>
							{:else}
								<img src="$lib/assets/icons/channel_text.svg" alt="text" class="filter_icon_main sidebar_channel_el_icon"/>
							{/if}
							{channels[i].name}
						</button>
					</div>
				{/each}
			{/if}
		{/if}
	</div>
	<div class="panel sidebar_message">
		{#if sel.channel > -1}
			{#snippet render_message(i, item)}
				<MessageDisplay id={item.id} text={item.text}
				author={users[item.author_id]}
				time_sent={new Date(item.sent)} time_edited={new Date(item.edited)}
				status={item.status}
				show_ctx_menu={(pos, action_set) => showCtxMenu(pos, action_set, i)}/>
			{/snippet}
			<PaginatedList bind:items={messages} bind:this={message_list}
			render_item={render_message} load_items={(index, count, _then) => {
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
</div>
