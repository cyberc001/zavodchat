<script>
	const {server} = $props();
</script>

<div class="server_head">
{server.data.name}
</div>

<style>
.server_head {
	height: 1lh;
	padding: 8px;

	border-width: 2px;
	border-style: none none solid none;
	border-color: var(--clr_border);

	font-size: 24px;
}
</style>
