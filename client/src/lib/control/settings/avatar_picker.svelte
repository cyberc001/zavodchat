<script>
	let {display_url = $bindable("")} = $props();

	import Button from "$lib/control/settings/button.svelte";

	let file_picker = $state();
	let value = $state("");
	let files = $state();
	$effect(() => {
		if(files){
			let file = files[0];
			const reader = new FileReader();
			reader.onload = (e) => {
				display_url = e.target.result;
			};
			reader.readAsDataURL(file);
		}
	});

	let prev_display_url;
	$effect(() => {
		// reset selected files if display_url was set to a link
		if(prev_display_url && prev_display_url.startsWith("data") && !display_url.startsWith("data")){
			files = null;
			value = "";
		}

		prev_display_url = display_url;
	});

	export function getFile(){
		// https://gist.github.com/ibreathebsb/a104a9297d5df4c8ae944a4ed149bcf1
		const arr = display_url.split(',');
		const mime = arr[0].match(/:(.*?);/)[1];
		const bstr = atob(arr[1]);
		let n = bstr.length;
		const u8_arr = new Uint8Array(n);
		for(; n > 0; --n)
			u8_arr[n - 1] = bstr.charCodeAt(n - 1);
		return new File([u8_arr], "", {type: mime});
	}
</script>

<div class="avatar_picker_frame">
	<img src={display_url} class="avatar_picker_img"/>
	<input style="display: none" type="file" accept=".png,.jpg,.bmp"
		bind:this={file_picker} bind:files bind:value/>
	<Button text="Choose file" onclick={() => file_picker.click()}
	--margin-bottom="0px"
	/>
</div>

<style>
@import "style.css";
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
