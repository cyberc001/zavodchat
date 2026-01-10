<script>
	let { name = "Group" } = $props();
</script>

<div style="margin: 0px 0px 18px 0px">
	{name}
	<div class="settings_group">
	<slot></slot>
	</div>
</div>

<style>
.settings_group {
	margin: 6px 0px 6px 0px;
	padding: 12px;

	border-style: solid;
	border-width: 2px;
	border-color: var(--clr_border);
}
</style>
