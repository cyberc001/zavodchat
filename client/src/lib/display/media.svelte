<script>
	import {asset} from '$app/paths';
	import {onDestroy} from 'svelte';

	let {close_media = () => {}} = $props();

	const onkeyup = (e) => {
		if(e.key === "Escape")
			close_media();
	};
	document.addEventListener("keyup", onkeyup);
	onDestroy(() => {
		document.removeEventListener("keyup", onkeyup);
	});
</script>

<div class="media_display">
	<slot></slot>
	<button
		class="hoverable transparent_button media_close_button"
		onclick={close_media}
	>
		<img src={asset("icons/close.svg")} alt="close media" class="filter_icon_main" style="width: 32px"/>
	</button>
</div>

<div class="media_bg">
</div>

<style>
.media_display {
	position: fixed;
	left: 50%;
	top: 50%;
	transform: translate(-50%, -50%);
	height: 90%;
	width: 90%;

	z-index: 110;
}
.media_bg {
	position: fixed;
	left: 0; right: 0; top: 0; bottom: 0;
	background: rgba(0, 0, 0, 0.8);

	z-index: 100;
}
.media_close_button {
	position: absolute;
	right: 0%;
}
</style>
