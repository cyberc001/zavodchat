<script>
	import Util from '$lib/util.js';
	import Textbox from '$lib/control/textbox.svelte';
	import Select from '$lib/control/select.svelte';

	let {label_text = "", error = $bindable(""),
		value = $bindable(""), expires = $bindable("")} = $props();

	let prev_expires;
	$effect(() => {
		if(expires !== prev_expires){
			if(expires === "never")
				value = "";
			else
				value = Math.floor((new Date(expires) - Date.now()) / Util.time_unit_mul(duration_units));
			prev_expires = expires;
		}
	});

	let duration_units = $state(Util.TimeUnits.Minutes);
</script>


{#snippet duration_select()}
	<Select options={[Util.TimeUnits.Minutes, Util.TimeUnits.Hours, Util.TimeUnits.Days]} option_labels={["min", "hr", "days"]}
		bind:value={() => duration_units,
			    (x) => {
					// Convert interval if its valid and units were changed
					if(x !== duration_units){
						const interval = parseInt(value, 10);
						if(!isNaN(interval) && interval > 0)
							value = Math.floor(interval * Util.time_unit_mul(duration_units) / Util.time_unit_mul(x));
					}

					duration_units = x;
				}
		}
		--margin-bottom="0px" --width="96px"/>
{/snippet}

<Textbox label_text={label_text} error={error}
		render_after={duration_select} --width="128px"
		bind:value={() => value,
			    (x) => {
				value = x;

				error = "";
				if(x === "")
					expires = prev_expires = "never";
				else {
					const interval = parseInt(x, 10);
					if(isNaN(interval))
						error = "Invalid number";
					else if(interval <= 0)
						error = "Non-positive integer";
					else
						expires = prev_expires = new Date(Date.now() + interval * Util.time_unit_mul(duration_units));
				}
			    }
		}
/>
