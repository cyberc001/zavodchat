<script>
	import {asset} from '$app/paths';

	let {message} = $props();

</script>


<div class="loading_page_panel">
	<div class="loading_page_message">
		<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main"
			style="width: 48px; margin-bottom: 8px"/>
		{message}
	</div>
</div>

<style>
.loading_page_panel {
	width: 100%;
	height: 100%;
}
.loading_page_message {
	position: absolute;
	top: 0;
	bottom: 0;
	left: 0;
	right: 0;
	margin: auto;
	width: fit-content;
	height: fit-content;

	display: flex;
	flex-direction: column;
	align-items: center;

	font-size: 18px;
}
</style>
