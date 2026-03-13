<script>
	import axios from 'axios';

	import {asset} from '$app/paths';
	import {onDestroy} from 'svelte';
	import Message from '$lib/rest/message.js';
	import Util from '$lib/util.js';

	let { max_rows = 5,
		value = $bindable(""), attachments = $bindable([]), links = $bindable([]),
		status, actions = [],
		onsend } = $props();

	let textarea_rows = $state(1);
	$effect(() => {
		let text_rows = 1;
		for(let i = 0; i < value.length; ++i)
			if(value[i] == '\n')
				++text_rows;
		textarea_rows = Math.min(text_rows, max_rows);
	});
	const textarea_onkeyup = (e) => {
		if(!status && e.code === "Enter" && e.ctrlKey
			&& (value.length > 0 || attachments.length > 0))
			onsend();
	}

	let files = $state();
	let file_input = $state();
	const onattach = () => {
		file_input.click();
	};

	$effect(() => {
		if(files){
			for(const file of files)
				attachments.push({type: file.type.startsWith("image") ? Message.AttachmentType.Image
						  : Message.AttachmentType.File,
						  content: file});
			files = undefined;
		}
	});

	// Detect links in value and turn them into attachments every 3 seconds
	let last_value_change;
	$effect(() => {
		value;
		if(typeof last_value_change === "undefined")
			last_value_change = new Date();
	});
	const link_intv = setInterval(() => {
		if(typeof last_value_change === "undefined" || new Date() - last_value_change < 500)
			return;
		last_value_change = undefined;
		console.log("SCANNING LINKS");

		// https://stackoverflow.com/questions/8188645/javascript-regex-to-match-a-url-in-a-field-of-text
		const re = /([-\.\w]+:\/{2,3})(?!.*[.]{2})(?![-.*\.])((?!.*@\.)[-_\w@^=%&:;~+\.]+(?<![-\.]))(\/[-_\w@^=%&$:;/~+\.]+(?<!\.))?[?]?([-_\w=&@$!|~+]+)*[#]?([-_\w=&@$!|~+]+)*/gi;
		const matches = [...value.matchAll(re)];

		Util.cancel_fetch_group("link_attachment");
		let links_left = matches.length;
		const new_links = [];
		for(const match of matches)
			if(match[0]){
				const link = match[0];
				Util.group_fetch("link_attachment", "/proxy", {headers: {link}},
				async (res) => {
					const html = await res.text();
					const meta = Util.get_page_meta(html);
					if(!meta){
						if(--links_left <= 0)
							links.splice(0, links.length, ...new_links);
						return;
					}
					new_links.push({content: link,
							title: meta.title,
							desc: meta.desc,
							type: Message.AttachmentType.Link});
					if(--links_left <= 0)
						links.splice(0, links.length, ...new_links);
				},
				() => {
					if(--links_left <= 0)
						links.splice(0, links.length, ...new_links);
				});
			} else
				--links_left;
		
		// No matches
		if(!links_left && !new_links.length)
			links.length = 0;
	}, 100);
	onDestroy(() => removeInterval(link_intv));
</script>

<div class="message_input">
	<input type="file" style="position: fixed; top: -100vh" bind:this={file_input} bind:files multiple/>

	<div class="message_input_center_panel">
		{#if status}
			<div class="message_input_status_panel">
				<img src={asset("icons/loading.svg")} alt="loading" class="filter_icon_main" style="margin-right: 5px"/>
				{status}
			</div>
		{/if}
		<div class="message_input_actions">
			{#each actions as a}
				<button class="item message_input_action_button" onclick={a.func}>{a.text}</button>
			{/each}
		</div>

		<div style="display: flex">
			<button class="hoverable transparent_button" onclick={onattach}>
				<img class="filter_icon_main" src={asset("icons/attachment.svg")}/>
			</button>
			<textarea class="item message_input_textarea" rows={textarea_rows}
				bind:value
				onkeyup={textarea_onkeyup}>
			</textarea>
		</div>

		<div style="display: flex; padding-top: 4px">
			{#each attachments as att, i}
				<div class="item message_input_attachment">
					<button class="attachment_remove_button hoverable item"
					onclick={() => attachments.splice(i, 1)}
					>
						<img src={asset("icons/close.svg")} alt="remove attachment" class="filter_icon_main" style="width: 32px"/>
					</button>
					{#if att.type === Message.AttachmentType.Image}
						<img src={URL.createObjectURL(att.content)} class="attachment_square"/>
					{:else}
						<div class="attachment_square">
							<img src={asset("icons/file.svg")} alt="file" class="filter_icon_main"
							style="width: 24px; margin-bottom: 4px"/>
							{att.content.name}
						</div>
					{/if}
				</div>
			{/each}
		</div>
		<div style="display: flex; flex-direction: column">
			{#each links as link, i}
				<div class="item link_attachment">
					<button class="attachment_remove_button hoverable item"
					onclick={() => links.splice(i, 1)}
					style="left: inherit; right: 0px"
					>
						<img src={asset("icons/close.svg")} alt="remove attachment" class="filter_icon_main" style="width: 32px"/>
					</button>
					<a href={link.content} class="link_attachment_line_limit"><b>{link.title}</b><br></a>
					<div class="line_limit">{link.desc}</div>
				</div>
			{/each}
		</div>
	</div>
</div>

<style>
.message_input {
	width: 100%;
	text-align: center;
	margin-bottom: 1%;
	margin-top: auto;
}
.message_input_center_panel {
	display: inline-block;
	width: 80%;
}

.message_input_actions {
	width: 100%;
	display: inline-block;
	text-align: left;
	margin-bottom: 6px;
}
.message_input_action_button {
	display: inline-block;

	border-color: var(--clr_border_item);
	border-radius: 4px;
	border-style: solid;

	font-size: 16px;
	color: var(--clr_text);
}
.message_input_action_button:active {
	background: var(--clr_bg_selected);
}


.message_input_status_panel {
	display: inline-flex;
	align-items: center;
	padding: 4px 6px 4px 4px;
	background: var(--clr_bg_item);
}

.message_input_textarea {
	width: 100%;
	resize: none;

	border-color: var(--clr_border_item);
	border-style: solid;
	border-width: 2px;
	border-radius: 4px;

	color: var(--clr_text);
	font-size: 16px;
}

.attachment_remove_button {
	position: absolute;
	top: 0;
	left: 0;

	padding: 0;
	border-radius: 0 0 4px 0
}
.message_input_attachment {
	position: relative;
	border-radius: 4px;
	padding: 6px;
	margin-right: 8px;
}
</style>
