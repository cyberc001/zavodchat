<script>
	let {track, close_video = () => {}} = $props();

	let elem = $state();
	$effect(() => {
		if(elem && track){
			elem.srcObject = new MediaStream([track]);
			elem.play();
		}
	});
</script>

<div class="video_display">
	<video class="video_container" bind:this={elem}/>
	<button
		class="hoverable transparent_button video_close_button"
		onclick={close_video}
		>
			<img src="$lib/assets/icons/close.svg" alt="close settings" class="filter_icon_main" style="width: 32px"/>
	</button>
</div>

<div class="video_bg">
</div>

<style>
.video_display {
	position: fixed;
	left: 50%;
	top: 50%;
	transform: translate(-50%, -50%);
	height: 90%;
	width: 90%;

	z-index: 110;
}
.video_container {
	position: absolute;
	width: 100%;
	height: 90vh;
}
.video_bg {
	position: fixed;
	left: 0; right: 0; top: 0; bottom: 0;
	background: rgba(0, 0, 0, 0.8);

	z-index: 100;
}
.video_close_button {
	position: absolute;
	right: 0%;
}
</style>
