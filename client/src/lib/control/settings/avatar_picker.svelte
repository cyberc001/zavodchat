<script>
	let {display_url = $bindable(""), file = $bindable()} = $props();

	let value = $state("");
	let files = $state();
	$effect(() => {
		if(files){
			file = files[0];
			const reader = new FileReader();
			reader.onload = (e) => {
				display_url = e.target.result;
			};
			reader.readAsDataURL(file);
		}
	});

	$effect(() => {
		console.log("display_url", display_url);
	});

	export function reset(){
		value = "";
		display_url = "";
	};
</script>

<div class="avatar_picker_frame">
	<img src={display_url} class="avatar_picker_img"/>
	<input type="file" bind:files bind:value/>
</div>

<style>
.avatar_picker_frame {
	
}
.avatar_picker_img {
	width: 64px;
}
</style>
