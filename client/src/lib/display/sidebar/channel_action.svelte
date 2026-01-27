<script>
	const {last, icon, text,
		action} = $props();
</script>

<div>
	<button class="item hoverable transparent_button sidebar_channel_el"
		style={(last ? "border-style: solid none solid none" : "")
			+ "; padding: 4px 0px 4px 4px"}
		onclick={action}
	>
		<img src={icon} class="filter_icon_main sidebar_channel_el_icon"/>
		{text}
	</button>
</div>

<style>
@import "channel_element.css";
</style>
