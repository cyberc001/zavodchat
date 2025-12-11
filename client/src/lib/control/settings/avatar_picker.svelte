<script>
	let {display_url = $bindable(""), file = $bindable()} = $props();

	import Button from "$lib/control/settings/button.svelte";

	let file_picker = $state();
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

	export function reset(){
		value = "";
		display_url = "";
	};
</script>

<div class="avatar_picker_frame">
	<img src={display_url} class="avatar_picker_img"/>
	<input style="display: none" type="file" accept=".png,.jpg,.bmp"
		bind:this={file_picker} bind:files bind:value/>
	<Button text="Choose file" onclick={() => file_picker.click()}/>
</div>

<style>
.avatar_picker_frame {
	display: flex;
	flex-direction: column;

	width: 132px;
}
.avatar_picker_img {
	width: 128px;

	margin-bottom: 4px;

	border-width: 2px;
	border-style: solid;
	border-color: var(--clr_border);
}
</style>
