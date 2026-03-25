<script>
	let {tabs} = $props();

	let sel_tab = $state(tabs[0]);
</script>

<div class="panel tab_panel">
	<div class="tab_button_panel">
		{#each tabs as tab}
			<button class="transparent_button hoverable tab_name"
			onclick={() => sel_tab = tab}>
				{tab.name}
			</button>
		{/each}
	</div>
	<div class="tab_content">
		{@render sel_tab.render(sel_tab.args)}
	</div>
</div>

<style>
.tab_panel {
	width: 100%;
	height: 100%;

	display: flex;
	flex-direction: column;
}
.tab_button_panel {
	margin-top: 6px;
}
.tab_content {
	margin-top: 12px;
	margin-left: 10px;
	margin-right: 10px;

	height: 100%;

	overflow-y: scroll;
}

.tab_name {
	font-size: 18px;

	padding: 2px;
	margin-left: 10px;
}

</style>
