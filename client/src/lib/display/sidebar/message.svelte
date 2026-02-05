<script>
	import {asset} from '$app/paths';

	import MessageDisplay from '$lib/display/message.svelte';
	import PaginatedList from '$lib/display/paginated_list.svelte';
	import MessageInput from '$lib/control/message_input.svelte';
	import ContextMenuAction from '$lib/control/context_menu_action.svelte';

	import Message from '$lib/rest/message.js';
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';

	let {server, channel,
		sel_message_id, sel_user_id,
		show_ctx_menu, show_user, show_ban} = $props();

	let server_roles = $state();
	$effect(() => {
		if(server > -1)
			server_roles = Role.get_list(server);
	});

	let sel = $state({
		message_edit: -1,
		ctx_message: -1,
		ctx_user_idx: -1
	});

	let message_list = $state();
	let message_text = $state("");
	let prev_message_text = "";
	let message_status = $state();

	const sendMessage = (text) => {
		message_status = "Sending...";
		Message.send(channel, text,
				() => {
					message_text = "";
					message_status = undefined;
				},
				() => {
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

	$effect(() => {
		channel;
		message_list?.reset();
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

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
				User.kick(server, sel.ctx_user_id, () => {}, () => {});
		}}
	/>
{/snippet}
{#snippet action_ban_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/ban.svg")} text="Ban"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => show_ban(sel.ctx_user_id)}
	/>
{/snippet}



<div class="panel sidebar_messages">
	{#if channel > -1}
		{#snippet render_message(i, item)}
			<MessageDisplay id={item.id} text={item.text}
			author={item.author}
			author_roles={item.author_roles}
			time_sent={new Date(item.sent)} time_edited={new Date(item.edited)}
			status={item.status}
			show_ctx_menu={(anchor, e, for_message) => {
				sel.ctx_user_id = item.author.data.id;
				sel.ctx_message = i;
				show_ctx_menu(anchor, e, for_message ? [action_edit_message, action_delete_message]
							: [action_kick_user, action_ban_user]);
			}}
			selected_user={item.id == sel_message_id && item.author_id == sel_user_id}
			onclick_user={() => show_user(item.author_id, item.id)}
			hide_profile={() => show_user(-1, -1)}
			/>
		{/snippet}
		<PaginatedList bind:this={message_list}
		reversed={true}
		loading_text="Loading messages..." to_latest_text="To latest messages"
		render_item={render_message}
		load_items={(index, range) => Message.get_range(channel, index, range)}
		augment_item={(msg) => {
					msg.author = User.get_server(server, msg.author_id);
					msg.author_roles = Role.get_user_roles(msg.author.data, server_roles.data);
		}}
		}}/>
		<MessageInput
			bind:value={message_text} onsend={sel.message_edit > -1 ? editMessage : sendMessage}
			status={message_status}
			actions={sel.message_edit > -1 ? [{text: "Stop editing", func: stopEditing}] : []}/>
	{/if}
</div>

<style>
.sidebar_messages {
	width: 100%;
	display: flex;
	flex-direction: column;
}
</style>
