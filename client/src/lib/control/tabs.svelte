<script>
	let {tabs} = $props();

	let sel_i = $state(0);
</script>

<div class="panel tab_panel">
	<div class="tab_button_panel">
		{#each tabs as tab, i}
			<button class={"transparent_button hoverable tab_name" + (sel_i === i ? " selected" : "")}
			onclick={() => sel_i = i}>
				{tab.name}
			</button>
		{/each}
	</div>
	<div class="tab_content">
		{@render tabs[sel_i].render(tabs[sel_i].args)}
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
