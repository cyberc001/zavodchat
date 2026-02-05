<script>
	let {element,
		onfocus = () => {}, onblur = () => {}
	} = $props();

	let mouse_in_element = true;
	// Changing element when its already not undefined will cause these event handlers to become bogus
	$effect(() => {
		if(element){
			element.onfocus = onfocus;
			element.onmouseenter = (e) => {
				mouse_in_element = true;
			};
			element.onmouseleave = (e) => {
				mouse_in_element = false;
			};
		}
	});
</script>

<svelte:window
onblurcapture={(e) => {
	// element got clicked
	if(!e.relatedTarget && mouse_in_element)
		return;

	if(!element.contains(e.relatedTarget))
		onblur();
}}
onmouseup={(e) => {
	if(!mouse_in_element)
		onblur();
}}
/>
