<script>
	import MessageDisplay from '$lib/display/message.svelte';
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
	let servers = $state({});
	let channels = $state({});
	let users = $state({});

	const ensureUser = (id) => {
		if(typeof users[id] === "undefined"){
			// TODO prevent double-loading
			User.get(id,
			(data) => {
				users[id] = data;
				console.log("First loaded user", $state.snapshot(users));
			}
			, setError);
		}
	};

	// UI state
	let sel = $state({
		server: -1, channel: -1
	});

	const action_sets = {
		"message": [{text: "Edit", icon: "edit.svg", func: () => {
				console.log("edit!!!");
			    }},
			    {text: "Delete", icon: "delete.svg", func: () => {
				console.log("delete!!!");
			    }}]
	};

	let ctx_menu_params = $state({
		visible: false,
		pos: [1000, 0],
		actions: []
	});
	const showCtxMenu = (pos, action_set) => {
		ctx_menu_params.pos = pos;
		ctx_menu_params.actions = action_sets[action_set];
		ctx_menu_params.visible = true;
	};
	const hideCtxMenu = () => {
		ctx_menu_params.visible = false;
	};

	// Events
	let showServer = (id) => {
		sel.server = id;
		sel.channel = -1;
		if(typeof servers[id].channels === "undefined"){
			// load all the channel data
			Channel.get_list(id,
					(list) => {
						servers[id].channels = [];
						for(let ch of list){
							servers[id].channels.push(ch.id);
							channels[ch.id] = ch;
						}
						console.log("First loaded channels", $state.snapshot(servers));
					},
					setError);
		}
	};
	let showChannel = (id) => {
		sel.channel = id;
		if(typeof channels[id].messages === "undefined"){
			// load last messages
			Message.get_range(sel.server, id, -1, -1,
						(list) => {
							channels[id].messages = list;
							for(let msg of list)
								ensureUser(msg.author_id);
							console.log("First loaded messages", $state.snapshot(channels));
						}
						, setError
			);
		}
	};

	// Initialization
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
	{#if ctx_menu_params.visible}
		<ContextMenu pos={ctx_menu_params.pos} hide_ctx_menu={hideCtxMenu}
			     actions={ctx_menu_params.actions}/>
	{/if}

	<div class="panel sidebar_servers">
		{#each Object.values(servers) as srv}
			<button class={"item hoverable sidebar_server" + (sel.server == srv.id ? " selected" : "")} onclick={() => showServer(srv.id)}>
			{#if srv.avatar === undefined}
				<div style="padding:4px;"><div class="sidebar_server_el">{srv.name}</div></div>
			{:else}
				<img class="sidebar_server_el" alt={srv.name} src={Server.get_avatar_path(srv)}/>
			{/if}
			</button>
		{/each}
	</div>
	<div class="panel sidebar_channel">
		{#if sel.server > -1}
			{#each servers[sel.server].channels as i}
				<div>
					<button class={"item hoverable sidebar_channel_el" + (sel.channel == i ? " selected" : "")} onclick={() => showChannel(i)}>
						{#if channels[i].type === 1}
							<img src="$lib/assets/icons/channel_vc.svg" alt="voice" class="sidebar_channel_el_icon"/>
						{:else}
							<img src="$lib/assets/icons/channel_text.svg" alt="text" class="sidebar_channel_el_icon"/>
						{/if}
						{channels[i].name}
					</button>
				</div>
			{/each}
		{/if}
	</div>
	<div class="panel sidebar_message">
		{#if sel.channel > -1}
			{#each channels[sel.channel].messages as msg}
				<MessageDisplay text={msg.text} author={users[msg.author_id]}
						show_ctx_menu={showCtxMenu}/>
			{/each}
		{/if}
	</div>
</div>
