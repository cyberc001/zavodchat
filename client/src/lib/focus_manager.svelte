<script>
	let {element,
		onfocus = () => {}, onblur = () => {},
		blur_on_mousedown = false
	} = $props();

	let mouse_in_element;
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

	const mouseblur = (e) => {
		if(mouse_in_element === false)
			onblur();
	};
</script>

<svelte:window
onblurcapture={(e) => {
	if(!element)
		return;

	// element got clicked
	if(!e.relatedTarget && mouse_in_element)
		return;

	if(!element.contains(e.relatedTarget))
		onblur();
}}
onmousedown={blur_on_mousedown ? mouseblur : () => {}}
onmouseup={blur_on_mousedown ? () => {} : mouseblur}
onmousemove={(e) => {
	if(typeof(mouse_in_element) === "undefined"){
		const elems = document.elementsFromPoint(e.clientX, e.clientY);
		for(const el of elems)
			if(element.contains(el)){
				mouse_in_element = true;
				break;
			}
		mouse_in_element = mouse_in_element || false;
	}
}}
/>
