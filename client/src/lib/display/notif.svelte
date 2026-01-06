<script>
	import Rest from '$lib/rest.svelte.js';

	import {fade} from 'svelte/transition';

	let {max_length = 10,
		fade_out_delay = 1000, fade_out_duration = 1000
	} = $props();

	let sorted_notifs = $derived.by(() => {
		let notifs = Object.values(Rest._inst.notifs);
		notifs.sort(function(a, b){
			if(a.timestamp < b.timestamp)
				return -1;
			else if(a.timestamp > b.timestamp)
				return 1;
			return 0;
		});
		if(notifs.length > max_length)
			notifs.splice(0, notifs.length - max_length);
		return notifs;
	});

	setInterval(() => {
		let now = Date.now();

		const keys = Object.keys(Rest._inst.notifs);
		for(const k of keys){
			let notif = Rest._inst.notifs[k];
			if(notif.type !== Rest.NotifTypes.Error)
				continue;

			if(now - notif.timestamp >= fade_out_delay + fade_out_duration)
				delete Rest._inst.notifs[k];
			else if(now - notif.timestamp >= fade_out_delay)
				notif.opacity = 1 - (now - notif.timestamp - fade_out_delay) / (fade_out_duration);
		}
	}, 33);

</script>

<div class="notif_display">
	{#each sorted_notifs as notif}
		<div
			class={"item notif_notif" + (notif.type === Rest.NotifTypes.Error ? " notif_notif_error" : "")}
			style={typeof notif.opacity !== "undefined" ? `opacity: ${notif.opacity}` : ""}
		>
			{#if notif.type === Rest.NotifTypes.Ongoing}
				<img src="$lib/assets/icons/loading.svg" alt="loading" class="filter_icon_main" style="width: 16px"/>
			{/if}
			{notif.message}
		</div>
	{/each}
</div>

<style>
.notif_display {
	position: fixed;
	top: 64px;
	left: 50%;
	transform: translate(-50%, 0%);

	display: flex;
	flex-direction: column;
	align-items: center;
}
.notif_notif {
	padding: 6px;
	margin-bottom: 6px;
	width: fit-content;

	font-size: 16px;
}
.notif_notif_error {
	color: var(--clr_text_error);
}
</style>
