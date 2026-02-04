<script>
	import Group from '$lib/control/group.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import RadioBox from '$lib/control/radiobox.svelte';
	import AvatarPicker from '$lib/control/avatar_picker.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Auth from '$lib/rest/auth.js';
	import User from '$lib/rest/user.svelte.js';
	import Preferences from '$lib/rest/preferences.svelte.js';

	let self_user = User.get(-1);
	let preferences = Preferences.data;

	// Profile
	let profile_passwords_match = $derived(state_profile.state.password === state_profile.state.password_repeat);
	class UserTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!self_user.loaded)
				return SettingsTabState.ChangesState.Loading;
			if(!profile_passwords_match)
				return SettingsTabState.ChangesState.Invalid;
		});

		apply_changes(){
			let changes = this.get_dict_of_changes();
			if(changes.avatar)
				changes.avatar = avatar_picker.getFile();
			else
				delete changes.avatar;

			Auth.change_user_data(changes,
				() => {
					super.discard_changes(["username", "password"]);
					super.apply_changes();
				},
				() => super.discard_changes());
		}
	};
	let state_profile = new UserTabState({displayname: "", avatar: "",
							username: "", password: "", password_repeat: ""});

	let avatar_picker = $state();

	$effect(() => {
		if(self_user.loaded){
			state_profile.set_default_state("displayname", self_user.data.name);
			state_profile.set_default_state("avatar", User.get_avatar_path(self_user.data));
		}
	});

	// Audio
	class AudioTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!preferences.loaded)
				return SettingsTabState.ChangesState.Loading;
		});

		apply_changes(){
			Preferences.change(super.get_dict_of_changes(),
						() => super.apply_changes(),
						() => super.discard_changes());
		}
	};
	let state_audio = new AudioTabState({noise_supression: "off"});
	$effect(() => {
		if(preferences.loaded)
			for(const key in preferences)
				state_audio.set_all_states(key, preferences[key]);
	});


	export function tabs() {
		return [
			{name: "Profile & Account", render: profile, state: state_profile},
			{name: "Audio", render: audio, state: state_audio}
		];
	}
</script>

{#snippet profile()}
<Group name="Profile settings">
	<div style="display: flex">
		<AvatarPicker bind:this={avatar_picker}
		bind:display_url={state_profile.state.avatar}
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

{#snippet audio()}
<Group name="Noise supression">
	<RadioBox text="Off" value="off" bind:group={state_audio.state.noise_supression}/>
	<RadioBox text="Browser" value="browser" bind:group={state_audio.state.noise_supression}/>
	<RadioBox text="RNNoise" value="rnnoise" bind:group={state_audio.state.noise_supression}/>
</Group>
{/snippet}
