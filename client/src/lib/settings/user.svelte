<script>
	import Textbox from "$lib/control/settings/textbox.svelte";
	import AvatarPicker from "$lib/control/settings/avatar_picker.svelte";	

	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";

	import Rest from "$lib/rest.js";
	import Auth from "$lib/rest/auth.js";
	import User from "$lib/rest/user.svelte.js";


	let self_user = User.get(-1);

	// Profile
	let state_profile = new SettingsTabState({displayname: "", username: "", password: "",
							avatar_file: null});
	let profile_avatar_picker = $state();
	let profile_avatar_url = $state(User.get_avatar_path(self_user));
	$effect(() => {
		console.log("updating", $state.snapshot(self_user));
		profile_avatar_url = User.get_avatar_path(self_user);
	});

	User.get_nocache(-1, (u) => {
		state_profile.set_all_states("displayname", u.name);
	});

	export function tabs() {
		return [
			{ name: "profile", render: profile, state: state_profile,
				apply_changes: () => {
					let req_body = {};
					if(state_profile.is_changed("displayname"))
						req_body.displayname = state_profile.state.displayname;
					if(state_profile.is_changed("username"))
						req_body.username = state_profile.state.username;
					if(state_profile.is_changed("password"))
						req_body.password = state_profile.state.password;
					if(state_profile.is_changed("avatar_file"))
						req_body.avatar = state_profile.state.avatar_file;
					Auth.change_user_data(req_body,
						() => {
							state_profile.discard_changes(["username", "password"]);
							state_profile.apply_changes();
							profile_avatar_picker.reset();
							profile_avatar_url = User.get_avatar_path(self_user);
						},
						() => state_profile.discard_changes());
			}}
		];
	}
</script>

{#snippet profile()}
<AvatarPicker bind:this={profile_avatar_picker}
bind:file={state_profile.state.avatar_file}
bind:display_url={profile_avatar_url}
/>
<Textbox label_text="Displayed name" bind:value={state_profile.state.displayname}/>
<Textbox label_text="Change username" bind:value={state_profile.state.username}/>
<Textbox label_text="Change password" is_password=true bind:value={state_profile.state.password}/>
{/snippet}
