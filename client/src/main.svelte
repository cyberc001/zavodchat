<script>
	import Rest from '$lib/rest.js';
	import Server from '$lib/rest/server.js';
	import Channel from '$lib/rest/channel.js';
	import Message from '$lib/rest/message.js';

	let { setPage } = $props();

	const setError = (err) => {
		window.alert(err.status + " " + err.data);
	};

	// Server data
	let servers = $state({});
	let channels = $state({});

	// UI state
	let sel = $state({
		server: -1, channel: -1
	});

	// Events
	let showServer = (id) => {
		sel.server = id;
		sel.channel = -1;
		if(servers[id].channels === undefined){
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
		if(channels[id].messages == undefined){
			// load last messages
			Message.get_range(sel.server, id, -1, -1,
						(list) => {
							channels[id].messages = list;
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
	<div class="panel sidebar_servers">
		{#each Object.values(servers) as srv}
			<button tabindex=0 class={"item hoverable sidebar_server" + (sel.server == srv.id ? " selected" : "")} onclick={() => showServer(srv.id)}>
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
</div>
