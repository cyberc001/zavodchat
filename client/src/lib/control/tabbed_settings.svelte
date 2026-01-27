<script>
	import SettingsTabState from "$lib/control/settings_tab_state.svelte.js";
	import Button from "$lib/control/button.svelte";

	let {tabs, close_settings} = $props();

	let sel_tab = $state(0);
</script>


<div class="panel tabbed_settings">
	<div class="tabbed_settings_sidebar">
		{#each tabs as tab, i}
			<div>
				<button
					class={"item hoverable transparent_button tabbed_settings_el" + (sel_tab == i ? " selected" : "")}
					onclick={() => sel_tab = i}
				>
					{tab.name}
				</button>
			</div>
		{/each}
	</div>
	<div class="tabbed_settings_tab">
	<div style="width: 100%">
		<div class="tabbed_settings_inner_tab">
			<div>
				{@render tabs[sel_tab].render(() => {
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
						<Button text="Discard changes" onclick={tabs[sel_tab].discard_changes}/>
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
	</div>
	<div style="z-index: 200">
		<button
		class="hoverable transparent_button"
		onclick={() => {
			if(tabs[sel_tab].state.reset)
				tabs[sel_tab].state.reset();
			close_settings();
		}}
		>
			<img src="$lib/assets/icons/close.svg" alt="close settings" class="filter_icon_main" style="width: 32px"/>
		</button>
	</div>
	{#if tabs[sel_tab].state.constructor.name === "SettingsTabState"
		&& tabs[sel_tab].state.changes === SettingsTabState.ChangesState.Loading}
		<div class="tabbed_settings_inner_tab_overlay">
			<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 48px"/>
		</div>
	{/if}
	</div>
</div>


<style>
.tabbed_settings {
	display: flex;

	width: 100%;
	height: 100%;
}

.tabbed_settings_el {
	border-style: solid none none none;

	width: 100%;
	padding: 4px 0px 4px 4px;

	color: var(--clr_text);
	font-size: 18px;

	display: inline-flex;
	text-align: left;
	align-items: center;

	overflow-wrap: anywhere;

	cursor: pointer;

	user-select: none;
}

.tabbed_settings_tab {
	position: relative;
	display: flex;

	width: 85%;
	padding: 16px;
}
.tabbed_settings_inner_tab {
	overflow: auto;
	padding-right: 16px;

	height: 100%;
	width: 100%;
}
.tabbed_settings_inner_tab_overlay {
	position: absolute;
	top: 0;
	left: 0;
	height: 100%;
	width: 100%;

	display: flex;
	align-items: center;
	justify-content: center;	

	z-index: 100;
	background: rgba(0, 0, 0, 0.5);
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
	right: 5%;

	margin-bottom: 32px;
	padding: 8px;

	border-style: solid;
	border-width: 2px;
	border-color: var(--clr_border);
	background: var(--clr_bg_panel);
}
</style>
