import {Marked} from 'marked';
import {markedHighlight} from "marked-highlight";
import hljs from 'highlight.js';

import User from '$lib/rest/user.svelte.js';
import Notifications from '$lib/rest/notifications.js';
import Role from '$lib/rest/role.js';

const __md_mention = {
	name: "mention",
	level: "inline",

	start(src){ return src.indexOf('@'); },
	tokenizer(src, tokens){
		// Match mention types without IDs
		let match = src.match(/^@([e])/);
		if(match){
			let text = match[0];
			switch(match[1]){
				case "e":
					text = "@everyone";
					break;
			}
			return {
				type: "mention",
				raw: match[0],
				text
			};
		}

		// Match mention types with IDs
		match = src.match(/^@([ur])(\d+)/);
		if(match){
			let text = match[0];
			const id = parseInt(match[2]);
			switch(match[1]){
				case "u":
					const user = User.get_server(Markdown.__server_id, id);
					if(!user.loaded)
						user.notify_on_load(Markdown.__reload);
					else
						text = `@${user.data.name}`;
					break;
				case "r":
					const role = Markdown.__server_roles.find((x) => x.id === id);
					if(role)
						text = `@${role.name}`;
					break;
			}
			return {
				type: "mention",
				raw: match[0],
				text
			};
		}

		return false;
	},
	renderer(token){
		if(token.raw === token.text)
			return token.raw;
		return `<span style="background: var(--clr_bg_selected)" data-raw-text="${token.raw}" data-expected-text="${token.text}">${token.text}</span>`;
	}
}

export default class Markdown {
	static marked = new Marked(
		markedHighlight({
			emptyLangClass: 'hljs',
			langPrefix: 'hljs language-',
			highlight(code, lang, info) {
				const language = hljs.getLanguage(lang) ? lang : 'plaintext';
				return hljs.highlight(code, {language}).value;
			}
		})
	);

	static __get_md_markers(token){
		const i = token.raw.indexOf(token.text);
		return [token.raw.substring(0, i), token.raw.substring(i + token.text.length)];
	}

	static marked_overlay = new Marked({
		breaks: true,

		renderer: {
			strong(token){
				const [m1, m2] = Markdown.__get_md_markers(token);
				return `<strong>${m1}${this.parser.parseInline(token.tokens)}${m2}</strong>`;
			},
			em(token){
				const [m1, m2] = Markdown.__get_md_markers(token);
				return `<em>${m1}${this.parser.parseInline(token.tokens)}${m2}</em>`;
			},
			del(token){
				const [m1, m2] = Markdown.__get_md_markers(token);
				return `<del>${m1}${this.parser.parseInline(token.tokens)}${m2}</del>`;
			},
			codespan(token){
				return `<code>${token.raw}</code>`;
			}
		},

		walkTokens(token){
			if(token.type === "link")
				this.links.push(token.text);
		},

		extensions: [__md_mention]
	});

	static parse(message, server_roles){
		let html = Markdown.marked.parse(message.text);

		// Add mentions from the message
		for(const m of message.mentions){
			const m_sstr = message.text.substring(m.begin_i, m.end_i + 1);
			let style = "";
			let _class = "mention";
			let user_id = "";
			let str = m_sstr;
			switch(m.type){
				case Notifications.MentionTypes.User:
					if(m.user?.loaded){
						str = `@${m.user.data.name}`;
						if(server_roles){
							const username_style = Role.get_username_style(Role.get_user_roles(m.user.data, server_roles));
							if(username_style.length > 0)
								style += username_style + ";";
						}
						style += `anchor-name: --user_mention_${message.id}_${m.begin_i};`
						_class += " user_mention";
						user_id = `id="user_mention_${m.id}"`;
					}
					break;
				case Notifications.MentionTypes.Role:
					if(server_roles){
						const role = server_roles.find((x) => x.id === m.id);
						if(role){
							str = `@${role.name}`;
							style += Role.get_color_style(role) + ";";
						}
					}
					break;
				case Notifications.MentionTypes.Everyone:
					str = "@everyone";
					break;
			}
			html = html.replace(m_sstr, `<button class="${_class}" style="${style}" ${user_id}>${str}</button>`);
		}
		return html;
	}

	static __server_roles;
	static parse_overlay(text, server_id, server_roles, reload){
		Markdown.marked_overlay.links = [];
		Markdown.__server_id = server_id;
		Markdown.__server_roles = server_roles;
		Markdown.__reload = reload;

		let html = Markdown.marked_overlay.parseInline(text);
		if(html.endsWith("\n"))
			html += "\n";
		html = html.replaceAll("\n", "<br>");
		return [html, Markdown.marked_overlay.links];
	}
};

Markdown.marked.use({
	renderer: {
		image(token){
			return token.raw;
		},
		blockquote(token){
			return `<blockquote>${this.parser.parse(token.tokens)}</blockquote>`;
		}
	}
});
