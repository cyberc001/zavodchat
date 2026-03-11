<script>
	import {asset} from '$app/paths';
	import {untrack} from 'svelte';

	import MessageDisplay from '$lib/display/message.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	import SearchBar from '$lib/control/search_bar.svelte';

	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import File from '$lib/rest/file.js';

	let {server_id, channel_id,
		sel_message_id, sel_user_id,
		show_ctx_menu, show_user, show_ban} = $props();

	let self_user = User.get(-1);

	let server_roles = $state();
	$effect(() => {
		if(server_id > -1)
			server_roles = Role.get_list(server_id);
	});

	let sel = $state({
		message_edit: -1,
		ctx_message: -1,
		ctx_user_idx: -1,
		highlight_message: -1
	});

	const _show_user = (user_id, message_id) => {
		sel.highlight_message = -1;
		show_user(user_id, message_id);
	};

	let highlight_expire_tout;
	const set_highlight_message = (id) => {
		sel.highlight_message = id;
		if(typeof highlight_expire_tout !== "undefined")
			clearTimeout(highlight_expire_tout);
		highlight_expire_tout = setTimeout(() => {
			highlight_expire_tout = undefined;
			sel.highlight_message = -1;
		}, 5000);
	}

	let message_list = $state();
	let search_bar = $state();

	let message_search_params = $state({});
	let is_search = $derived(Object.keys(message_search_params).length > 0);

	let message_text = $state("");
	let prev_message_text = "";
	let message_status = $state();
	let message_attachments = $state([]);

	const resetMessage = (keep_content) => {
		if(!keep_content){
			message_text = "";
			message_attachments = [];
		}
		message_status = undefined;
	};
	const compileMessage = (_then, _catch) => {
		let attachments = [];
		for(const att of message_attachments)
			attachments.push(Object.assign({}, att));
		let msg = {
			text: message_text,
			attachments
		};

		// TODO change counting when handling links
		console.log("SENDING ATTACHMENTS", $state.snapshot(msg.attachments));
		let to_upload = msg.attachments.length;
		for(let att of msg.attachments)
			switch(att.type){
				case Message.AttachmentType.Image:
					File.upload(att.content, (res) => {
						att.content = `/files/upload/${self_user.data.id}/${res.data}`;
						if(--to_upload === 0)
							_then(JSON.stringify(msg));
					},  _catch);
					break;
			}
	};

	const sendMessage = () => {
		const _then = () => resetMessage();
		const _catch = () => resetMessage(true);

		message_status = "Sending...";
		compileMessage((data) => Message.send(channel_id, data, _then, _catch),
				_catch);
	};

	const editMessage = () => {
		const _then = () => {
			resetMessage();
			msg.status = Message.Status.None;
		};
		const _catch = () => {
			resetMessage(true);
			msg.status = Message.Status.None;
			msg.text = prev_text;
		};

		let msg = message_list.getItem(sel.message_edit);
		let prev_text = msg.text;
		msg.status = Message.Status.Editing;
		sel.message_edit = -1;

		compileMessage((data) => Message.edit(msg.id, data, _then, _catch),
				_catch);
	};
	const stopEditing = () => {
		sel.message_edit = -1;
		message_text = prev_message_text;
	};

	$effect(() => {
		$inspect.trace();
		channel_id;
		if(channel_id > -1)
			untrack(() => message_list?.reset());
	});
</script>


{#snippet action_edit_message(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/edit.svg")} text="Edit"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			sel.message_edit = sel.ctx_message;
			prev_message_text = message_text;
			message_text = message_list.getItem(sel.ctx_message).text;
		}}
	/>
{/snippet}
{#snippet action_delete_message(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/delete.svg")} text="Delete"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			let msg = message_list.getItem(sel.ctx_message);
			msg.status = Message.Status.Deleting;
			Message.delete(msg.id,
					() => {}, () => msg.status = Message.Status.None);
		}}
	/>
{/snippet}
{#snippet action_goto_message(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Go to"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			message_search_params = {};
			search_bar.reset();
			const msg = message_list.getItem(sel.ctx_message);
			console.log("GO TO", msg);
			set_highlight_message(msg.id);
			message_list.reset(msg.id);
		}}
	/>
{/snippet}

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			User.kick(server_id, sel.ctx_user_id, () => {}, () => {});
		}}
	/>
{/snippet}
{#snippet action_ban_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/ban.svg")} text="Ban"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => show_ban(sel.ctx_user_id)}
	/>
{/snippet}



<div class="panel sidebar_message">
	{#if channel_id > -1}
	<div style="height: 100%; position: relative">
		<div class="sidebar_message_content">
			{#snippet render_message(i, item)}
				<MessageDisplay data={item}
				show_ctx_menu={(anchor, e, for_message) => {
					sel.ctx_user_id = item.author.data.id;
					sel.ctx_message = i;
					_show_user(-1, item.id);
					show_ctx_menu(anchor, e, for_message ?
								(is_search ? [action_goto_message]
								: [action_edit_message, action_delete_message])
								: [action_kick_user, action_ban_user]);
				}}
				selected={item.id === sel_message_id || item.id === sel.message_edit}
				selected_user={item.id === sel_message_id && item.author_id === sel_user_id}
				highlighted={item.id === sel.highlight_message}
				onclick_user={() => _show_user(item.author_id, item.id)}
				hide_profile={() => _show_user(-1, -1)}
				/>
			{/snippet}
			<PaginatedList bind:this={message_list}
			reversed={true}
			loading_text="Loading messages..." to_latest_text="To latest messages"
			render_item={render_message}
			load_items={(start_id, range, asc) => Message.get_search_range(channel_id, start_id, range, asc, message_search_params)}
			augment_item={(msg) => {
					msg.author = User.get_server(server_id, msg.author_id);
					if(server_roles)
						msg.author_roles = Role.get_user_roles(msg.author.data, server_roles.data);
			}}
			/>
			<MessageInput
				bind:value={message_text} bind:attachments = {message_attachments}
				onsend={sel.message_edit > -1 ? editMessage : sendMessage}
				status={message_status}
				actions={sel.message_edit > -1 ? [{text: "Stop editing", func: stopEditing}] : []}
			/>
		</div>

		<div class="sidebar_message_search">
			<SearchBar server_id={server_id} bind:this={search_bar}
				elements={[
					{type: "server_user", label: "Author", param: "author_id"},
					{type: "date", label: "Date from", param: "date_from"},
					{type: "date", label: "Date until", param: "date_until"}
				]}
				onsearch={(params) => {
					sel.highlight_message = -1;
					message_search_params = params;
					message_list.reset();
				}}
			/>
		</div>
	</div>
	{/if}
</div>

<style>
.sidebar_message {
	width: 100%;
}
.sidebar_message_content {
	height: 100%;

	display: flex;
	flex-direction: column;
}
.sidebar_message_search {
	top: 10px;
	right: 10px;

	position: absolute;
}
</style>
