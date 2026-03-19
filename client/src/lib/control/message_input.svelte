<script>
	import axios from 'axios';

	import {asset} from '$app/paths';
	import {onDestroy} from 'svelte';
	import Markdown from '$lib/display/markdown.js';
	import Select from '$lib/select.js';

	import Util from '$lib/util.js';

	import Message from '$lib/rest/message.js';
	import Params from '$lib/rest/params.js';
	import File from '$lib/rest/file.js';

	let {value = $bindable(), attachments = $bindable([]), links = $bindable([]),
		status, actions = [],
		onsend } = $props();

	const div_input = (e) => {
		console.log("SELECTION", window.getSelection().getRangeAt(0));

		let sel_i = Select.get_selection_index(e.target);
		console.log("event", e);
		console.log("sel_i", sel_i);

		value = Select.get_inner_text(e.target);
		if(value.endsWith("\n"))
			value = value.substring(0, value.length - 1);
		console.log(`got value:\n'${value}'\ngot inner html:\n${e.target.innerHTML}`);
		[e.target.innerHTML, link_candidates] = Markdown.parse(value, true);
		link_candidates_ts = new Date();
		console.log(`new value:\n'${value}' ${value.split("\n").length - 1}\nnew html:\n'${e.target.innerHTML}'`);

		Select.set_selection_index(e.target, sel_i);
	
		e.preventDefault();
	};
	let value_html = $derived(Markdown.parse(value, true));

	const div_onkeyup = (e) => {
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
			for(const file of files){
				if(attachments.length >= Params.data.max_attachments)
					break;
				attachments.push({type: file.type.startsWith("image") ? Message.AttachmentType.Image
						  : Message.AttachmentType.File,
						  content: file});
			}
			files = undefined;
			links.splice(Params.data.max_attachments - attachments.length);
		}
	});

	let link_candidates;
	let link_candidates_ts = 0;
	const link_intv = setInterval(() => {
		if(!link_candidates || new Date() - link_candidates_ts < 500)
			return;

		Util.cancel_fetch_group("link_attachment");
		let links_left = link_candidates.length;
		const new_links = [];

		const swap_links = () => {
			if(new_links.length === 0)
				return;
			if(--links_left <= 0 || attachments.length + new_links.length > Params.data.max_attachments){
				console.log("new links", JSON.parse(JSON.stringify(new_links)), Params.data.max_attachments - attachments.length);
				new_links.splice(Params.data.max_attachments - attachments.length);
				console.log("spliced new links", new_links);
				links.splice(0, links.length, ...new_links);
				new_links.length = 0;
			}
		};

		for(const link of link_candidates){
			Util.group_fetch("link_attachment", "/proxy", {headers: {link}},
				async (res) => {
					const html = await res.text();
					const meta = Util.get_page_meta(html);
					if(!meta){
						swap_links();
						return;
					}
					console.log("GOT PAGE META", meta);
					new_links.push({content: link,
							title: typeof meta === "string" ? meta : meta.title,
							desc: meta.desc,
							type: Message.AttachmentType.Link});
					swap_links();
				},
				() => swap_links()
			);
		}

		if(!link_candidates.length)
			links = [];
		link_candidates = undefined;
	}, 100);
	onDestroy(() => clearInterval(link_intv));
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
			<div contenteditable="true" class="item message_input_div" id="message_input"
				oninput={div_input}
				onkeyup={div_onkeyup}>
			</div>
		</div>

		<div style="display: flex; padding-top: 4px; flex-wrap: wrap">
			{#each attachments as att, i}
				<div class="item message_input_attachment">
					<button class="attachment_remove_button hoverable item"
					onclick={() => {
						attachments.splice(i, 1);
						// parse links again, in case there were hitting the attachment limit before
						last_value_change = new Date();
					}}
					>
						<img src={asset("icons/close.svg")} alt="remove attachment" class="filter_icon_main" style="width: 32px"/>
					</button>
					{#if att.type === Message.AttachmentType.Image}
						<img src={typeof att.content === "string" ? File.get_attachment_url(att.content) : URL.createObjectURL(att.content)} class="attachment_square"/>
					{:else}
						<div class="attachment_square">
							<img src={asset("icons/file.svg")} alt="file" class="filter_icon_main"
							style="width: 24px; margin-bottom: 4px"/>
							{typeof att.content === "string" ? Util.get_file_name(att.content) : att.content.name}
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
						<img src={asset("icons/close.svg")} alt="remove link attachment" class="filter_icon_main" style="height: 28px"/>
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

.message_input_div {
	width: 100%;
	resize: none;

	max-height: 5lh;
	overflow-y: scroll;

	text-align: left;

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
