<script>
	import Group from '$lib/control/settings/group.svelte';
	import Textbox from '$lib/control/settings/textbox.svelte';
	import AvatarPicker from '$lib/control/settings/avatar_picker.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Auth from '$lib/rest/auth.js';
	import User from '$lib/rest/user.svelte.js';


	let self_user = User.get(-1);

	// Profile
	let state_profile = new SettingsTabState({displayname: "", avatar: null,
							username: "", password: "", password_repeat: ""});

	let profile_avatar_picker = $state();
	let profile_avatar_url = $state("");
	let profile_passwords_match = $derived(state_profile.state.password === state_profile.state.password_repeat);

	$effect(() => {
		state_profile.changes_override = profile_passwords_match ? SettingsTabState.ChangesState.Inherit : SettingsTabState.ChangesState.Invalid;
	});

	$effect(() => {
		if(self_user.loaded){
			profile_avatar_url = User.get_avatar_path(self_user.data);
			state_profile.set_all_states("displayname", self_user.data.name);
		}
	});

	export function tabs() {
		return [
			{ name: "Profile & Account", render: profile, state: state_profile,
				apply_changes: () => {
					if(!profile_passwords_match)
						return;

					Auth.change_user_data(state_profile.get_dict_of_changes(),
						() => {
							state_profile.discard_changes(["username", "password"]);
							state_profile.apply_changes();
							profile_avatar_picker.reset();
							profile_avatar_url = User.get_avatar_path(self_user.data);
						},
						() => state_profile.discard_changes());
			}}
		];
	}
</script>

{#snippet profile()}
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker bind:this={profile_avatar_picker}
		bind:file={state_profile.state.avatar}
			bind:display_url={profile_avatar_url}
		/>
		<div style="margin-left: 16px"></div>
	<Textbox label_text="Displayed name" bind:value={state_profile.state.displayname} --width="363px"/>
	</div>
</Group>
<Group name="Account settings">
	<Textbox label_text="Change username" bind:value={state_profile.state.username}/>
	<Textbox label_text="Change password" is_password=true bind:value={state_profile.state.password}/>
	<Textbox label_text="Repeat password" is_password=true bind:value={state_profile.state.password_repeat}
		error={profile_passwords_match ? "" : "Passwords do not match"}/>
</Group>
{/snippet}
