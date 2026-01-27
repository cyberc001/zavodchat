<script>
	const {icon, action} = $props();
</script>

<button class={"item hoverable sidebar_server"}
	onclick={action}
>
	<img src={icon} class="filter_icon_main" style="width: 60%"/>
</button>

<style>
@import "server_element.css";
</style>
