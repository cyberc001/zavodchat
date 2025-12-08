<script>
	let {tabs, close_settings} = $props();

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
		{@render tabs[sel_tab].render()}
	</div>
	<div>
		<button
		class="hoverable transparent_button"
		onclick={close_settings}
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
	width: 90%;
	padding: 16px;
}

.tabbed_settings_sidebar {
	border-style: none solid none none;
	border-color: var(--clr_border);
	border-width: 2px;

	width: 10%;
}
</style>
