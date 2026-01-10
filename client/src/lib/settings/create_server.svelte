<script>
	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import AvatarPicker from '$lib/control/avatar_picker.svelte';

	import CreateTabState from '$lib/control/create_tab_state.svelte.js';

	import Util from '$lib/util.js';
	import Server from '$lib/rest/server.js';

	// General
	class CreateTabStateGeneral extends CreateTabState {
		valid = $derived(this.state.name.length > 0);
	};
	let state_general = new CreateTabStateGeneral({name: "", avatar: ""});

	let avatar_picker = $state();

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				create: (close_settings) => {
					let data = Util.object_from_object(state_general.state);
					const avatar_url = data.avatar;
					if(data.avatar)
						data.avatar = avatar_picker.getFile();
					else
						delete data.avatar;
					
					Server.create(data,
						(server_id) => {
							data.id = server_id;
							if(avatar_url)
								data.avatar = avatar_url;
							Server.server_list_cache.get_state(0).data.push(data);
							close_settings();
						},
						() => {});
				}
			}
		];
	}
</script>

{#snippet general(params, close_settings)}
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker
		bind:this={avatar_picker}
		bind:display_url={state_general.state.avatar}
		/>
		<div style="margin-left: 16px"></div>
	<Textbox label_text="Server name" bind:value={state_general.state.name} --width="363px"/>
	</div>
</Group>
{/snippet}
