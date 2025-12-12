<script>
	import Group from "$lib/control/settings/group.svelte";
	import Textbox from "$lib/control/settings/textbox.svelte";
	import AvatarPicker from "$lib/control/settings/avatar_picker.svelte";

	import CreateTabState from "$lib/control/create_tab_state.svelte.js";

	import Util from "$lib/util.js";
	import Rest from "$lib/rest.js";
	import Server from "$lib/rest/server.js";

	// General
	class CreateTabStateGeneral extends CreateTabState {
		valid = $derived(this.state.name.length > 0);
	};
	let state_general = new CreateTabStateGeneral({name: "", avatar: null});

	let server_avatar_url = $state("");

	export function tabs() {
		return [
			{ name: "General", render: general, state: state_general,
				create: (close_settings) => {
					let data = Util.object_from_object(state_general.state);
					
					Server.create(state_general.state,
						(server_id) => {
							data.id = server_id;
							data.avatar = server_avatar_url;
							Server.server_list_cache.cache[0].push(data);
							close_settings();
						},
						() => {});
				}
			}
		];
	}
</script>

{#snippet general(p)}
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker
		bind:file={state_general.state.avatar}
		bind:display_url={server_avatar_url}
		/>
		<div style="margin-left: 16px"></div>
	<Textbox label_text="Server name" bind:value={state_general.state.name} --width="363px"/>
	</div>
</Group>
{/snippet}
