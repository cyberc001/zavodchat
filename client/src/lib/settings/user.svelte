<script>
	import Group from '$lib/control/group.svelte';
	import List from '$lib/control/list.svelte';
	import Textbox from '$lib/control/textbox.svelte';
	import RadioBox from '$lib/control/radiobox.svelte';
	import Button from '$lib/control/button.svelte';
	import AvatarPicker from '$lib/control/avatar_picker.svelte';
	import UserDisplay from '$lib/display/user.svelte';

	import SettingsTabState from '$lib/control/settings_tab_state.svelte.js';

	import Auth from '$lib/rest/auth.js';
	import User from '$lib/rest/user.svelte.js';
	import Preferences from '$lib/rest/preferences.svelte.js';
	import BlockedUsers from '$lib/rest/blocked_users.js';

	let self_user = User.get(-1);
	let preferences = Preferences.data;
	let blocked_users = BlockedUsers.get();

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

	// Other users
	class OtherUsersTabState extends SettingsTabState {
		changes_override = $derived.by(() => {
			if(!blocked_users.loaded)
				return SettingsTabState.ChangesState.Loading;
		});

		apply_changes(){
			this.execute_list_changes("blocked_users",
				undefined, undefined,
				(user, _then, _catch) => BlockedUsers.unblock_user(user.id, _then, _catch),
				() => {}, () => {}
			);
		}	
	};
	let state_other_users = new OtherUsersTabState({blocked_users: []});
	$effect(() => {
		if(blocked_users.loaded)
			state_other_users.set_default_state("blocked_users", blocked_users.data);
	});

	let blocked_users_list_selected_idx = $state(-1);


	export function tabs() {
		return [
			{name: "Profile & Account", render: profile, state: state_profile},
			{name: "Audio", render: audio, state: state_audio},
			{name: "Other users", render: other_users, state: state_other_users}
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


{#snippet render_blocked_user(i, item)}
	<UserDisplay user={{data: item, loaded: true}} display_status={false}/>
{/snippet}

{#snippet other_users()}
<Group name="Blocked users">
<List items={state_other_users.state.blocked_users}
	render_item={render_blocked_user}
	bind:selected_idx={blocked_users_list_selected_idx}
/>
<Button text="Unblock user"
	disabled={blocked_users_list_selected_idx < 0}
	onclick={() => {
			state_other_users.state.blocked_users.splice(blocked_users_list_selected_idx, 1);
			blocked_users_list_selected_idx = -1;
	}}/>

</Group>
{/snippet}
