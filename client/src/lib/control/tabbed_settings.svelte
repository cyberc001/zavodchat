<script>
	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";
	import Button from "$lib/control/settings/button.svelte";

	let {tabs, params, close_settings} = $props();

	let sel_tab = $state(0);
</script>


<div class="panel tabbed_settings">
	<div class="tabbed_settings_sidebar">
		{#each tabs as tab, i}
			<div>
				<button
				class={"item hoverable transparent_button sidebar_channel_el" + (sel_tab == i ? " selected" : "")}
				onclick={() => sel_tab = i}
				>
					{tab.name}
				</button>
			</div>
		{/each}
	</div>
	<div class="tabbed_settings_tab">
		<div>{@render tabs[sel_tab].render(params, () => {
				if(tabs[sel_tab].state.reset)
					tabs[sel_tab].state.reset();
				close_settings();
			})}
		</div>

		{#if tabs[sel_tab].state.constructor.name === "SettingsTabState"}
			{#if tabs[sel_tab].state.changes === SettingsTabState.ChangesState.HasChanges
				|| tabs[sel_tab].state.changes === SettingsTabState.ChangesState.Invalid}
				<div class="tabbed_settings_actions">
					<Button text="Apply changes" onclick={tabs[sel_tab].apply_changes} disabled={tabs[sel_tab].state.changes === SettingsTabState.ChangesState.Invalid}/>
					<Button text="Discard changes" onclick={() => tabs[sel_tab].state.discard_changes()}/>
				</div>
			{/if}
		{:else}
			<div class="tabbed_settings_actions">
				<Button text="Create"
				onclick={() => tabs[sel_tab].create(() => {
					tabs[sel_tab].state.reset();
					close_settings();
				})}
				disabled={!tabs[sel_tab].state.valid}/>
			</div>
		{/if}
	</div>
	<div>
		<button
		class="hoverable transparent_button"
		onclick={() => {
			if(tabs[sel_tab].state.constructor.name !== "SettingsTabState")
				tabs[sel_tab].state.reset();
			close_settings();
		}}
		>
			<img src="$lib/assets/icons/close.svg" alt="close settings" class="filter_icon_main" style="width: 32px"/>
		</button>
	</div>
</div>


<style>
.tabbed_settings {
	display: flex;

	width: 100%;
	height: 100%;
}

.tabbed_settings_tab {
	position: relative;

	width: 85%;
	padding: 16px;
}

.tabbed_settings_sidebar {
	border-style: none solid none none;
	border-color: var(--clr_border);
	border-width: 2px;

	width: 15%;
}

.tabbed_settings_actions {
	position: absolute;
	bottom: 0%;

	margin-bottom: 32px;
	padding: 8px;

	border-style: solid;
	border-width: 2px;
	border-color: var(--clr_border);
}
</style>
