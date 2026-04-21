<script>
	import User from '$lib/rest/user.svelte.js';
	import Role from '$lib/rest/role.js';
	import BlockedUsers from '$lib/rest/blocked_users.js';

	import ContextMenuAction from '$lib/control/context_menu_action.svelte';
	import {asset} from '$app/paths';

	let {show_ban} = $props();

	let server = $state();
	let other_user_id = $state();

	export function get(user_id, _other_user_id, _server, server_roles,
				_then){
		let user, other_user;
		if(_server){
			user = User.get_server(_server.data.id, user_id);
			other_user = User.get_server(_server.data.id, _other_user_id);
		} else {
			user = User.get(user_id);
			other_user = User.get(_other_user_id);
		}

		user.notify_on_load(() => {
			other_user.notify_on_load(() => {
				server = _server;
				other_user_id = _other_user_id;

				let actions = [action_block_user];
				if(server && Role.check_lower_role(user.data, other_user.data, _server.data, server_roles.data)){
					if(Role.check_perms(user.data, _server.data, server_roles.data, 1, 3))
						actions.push(action_kick_user);
					if(Role.check_perms(user.data, _server.data, server_roles.data, 1, 4))
						actions.push(action_ban_user);
				}
				_then(actions);
			});
		});
	}
</script>

{#snippet action_kick_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/kick.svg")} text="Kick"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => {
			User.kick(server.data.id, other_user_id, () => {}, () => {});
		}}
	/>
{/snippet}
{#snippet action_ban_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/ban.svg")} text="Ban"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => show_ban(other_user_id)}
	/>
{/snippet}
{#snippet action_block_user(hide_ctx_menu)}
	<ContextMenuAction icon={asset("icons/block.svg")} text="Block"
		hide_ctx_menu={hide_ctx_menu}
		onclick={() => BlockedUsers.block_user(other_user_id,
							() => {}, () => {})}
	/>
{/snippet}
