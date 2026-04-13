<script>
	import {asset} from '$app/paths';
	import {untrack} from 'svelte';

	import Util from '$lib/util.js';
	import MessageDisplay from '$lib/display/message.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	import ChannelHead from '$lib/display/sidebar/channel_head.svelte';
	import PrivateCall from '$lib/control/private_call.svelte';

	import Message from '$lib/rest/message.js';
	import Channel from '$lib/rest/channel.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import File from '$lib/rest/file.js';

	let {server, channel_id,
		sel_message_id, sel_user_id,
		socket_vc,
		show_ctx_menu, ctx_vc_user, show_user, show_ban,
		show_channel, end_call} = $props();

	let self_user = User.get(-1);

	let sel = $state({
		message_edit: -1,
		ctx_message: -1,
		ctx_user_idx: -1,
		highlight_message: -1
	});

	const _show_user = (user_id, anchor, anchor_side_x) => {
		sel.highlight_message = -1;
		show_user(user_id, anchor, anchor_side_x);
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

	let channel_head = $state();
	let message_list = $state();

	let message_search_params = $state({});
	let is_search = $derived(Object.keys(message_search_params).length > 0);

	let message_text = $state("");
	let message_status = $state();
	let message_attachments = $state([]);
	let message_links = $state([]);

	const resetMessage = (keep_content) => {
		if(!keep_content){
			message_text = "";
			message_attachments = [];
			message_links = [];
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

		let to_upload = message_attachments.length;
		for(const att of msg.attachments){
			if(typeof att.content === "string")
				--to_upload;
			else
				File.upload(att.content, (res) => {
					att.content = `/files/upload/${self_user.data.id}/${res.data}`;
					if(--to_upload === 0)
						_then(msg);
				},  _catch);
		}

		for(const link of message_links)
			attachments.push(Util.object_from_object(link, ["type", "content"]));

		if(!to_upload)
			_then(msg);
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
			msg.attachments = prev_attachments;
		};

		message_list.set_anchor(sel.message_edit);
		const msg = message_list.getItem(sel.message_edit);
		const prev_text = msg.text;
		const prev_attachments = msg.attachments;
		msg.status = Message.Status.Editing;
		sel.message_edit = -1;

		compileMessage((data) => Message.edit(msg.id, data, _then, _catch),
				_catch);
	};
	const stopEditing = () => {
		sel.message_edit = -1;
		resetMessage();
	};

	// Reset scroll when changing channels
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
			const msg = message_list.getItem(sel.ctx_message);
			message_text = msg.text;
			message_attachments = msg.attachments.filter((x) => x.type !== Message.AttachmentType.Link);
		}}
	/>
{/snippet}
{#snippet action_delete_message(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/delete.svg")} text="Delete"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			let msg = message_list.getItem(sel.ctx_message);
			message_list.set_anchor(sel.ctx_message);
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
			channel_head.reset();
			const msg = message_list.getItem(sel.ctx_message);
			set_highlight_message(msg.id);
			message_list.reset(msg.id);
		}}
	/>
{/snippet}

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			User.kick(server.data.id, sel.ctx_user_id, () => {}, () => {});
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
	<div style="height: 100%; position: relative; display: flex; flex-direction: column">
		<ChannelHead bind:this={channel_head}
		channel={Channel.get(channel_id)} server={server}
		onsearch={(params) => {
			sel.highlight_message = -1;
			message_search_params = params;
			message_list.reset();
		}}
		show_channel={show_channel}
		/>

		<PrivateCall socket_vc={socket_vc}
				end_call={end_call} ctx_vc_user={ctx_vc_user}/>

		<div class="sidebar_message_content">
			{#snippet render_message(i, item)}
				<MessageDisplay data={item} server={server}
				show_ctx_menu={(anchor, e, for_message) => {
					sel.ctx_user_id = item.author.data.id;
					sel.ctx_message = i;
					_show_user(-1);
					show_ctx_menu(anchor, e, for_message ?
								(is_search ? [action_goto_message]
								: [action_edit_message, action_delete_message])
								: [action_kick_user, action_ban_user]);
				}}
				selected={item.id === sel_message_id || item.id === sel.message_edit}
				selected_user={item.id === sel_message_id && item.author_id === sel_user_id}
				highlighted={item.id === sel.highlight_message}
				show_user={_show_user}
				/>
			{/snippet}
			<PaginatedList bind:this={message_list}
			reversed={true}
			loading_text="Loading messages..." to_latest_text="To latest messages"
			render_item={render_message}
			load_items={(start_id, range, asc) => Message.get_search_range(channel_id, start_id, range, asc, message_search_params)}
			augment_item={(msg) => {
				msg.author = server ? User.get_server(server.data.id, msg.author_id) : User.get(msg.author_id);
			}}
			/>
			<MessageInput
				bind:value={message_text} bind:attachments={message_attachments} bind:links={message_links}
				server={server}
				onsend={sel.message_edit > -1 ? editMessage : sendMessage}
				status={message_status}
				actions={sel.message_edit > -1 ? [{text: "Stop editing", func: stopEditing}] : []}
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
	flex-grow: 1;
	min-height: 0;

	display: flex;
	flex-direction: column;
}
.sidebar_message_search {
	top: 10px;
	right: 10px;

	position: absolute;
}
</style>
