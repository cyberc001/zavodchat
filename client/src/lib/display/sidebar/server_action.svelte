<script>
	const {icon, action,
		notif_count} = $props();
</script>

<button class={"item hoverable sidebar_server"}
	style="position: relative"
	onclick={action}
>
	<img src={icon} class="filter_icon_main" style="width: 60%"/>
	{#if notif_count}
		<div class="notif_circle">{notif_count}</div>
	{/if}
</button>

<style>
@import "server_element.css";
</style>
