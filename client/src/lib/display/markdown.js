import {Marked} from 'marked';
import {markedHighlight} from "marked-highlight";
import hljs from 'highlight.js';

/*const __md_mention = {
	name: "mention",
	level: "inline",

	start(src){ return src.indexOf('@'); },
	tokenizer(src, tokens){
		const match = src.match(/^@u\d+/);
		if(match)
			return {
				type: "mention",
				raw: match[0],
				text: match[0]
			};
		return false;
	},
	renderer(token){
		return `<span style="background: var(--clr_bg_selected)">${token.raw}</span>`;
	}
}*/

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

		//extensions: [__md_mention],
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
		}
	});

	static parse(message, overlay){
		Markdown.marked_overlay.links = [];
		let html = overlay ? Markdown.marked_overlay.parseInline(message) :
				     Markdown.marked.parse(message.text);
		if(overlay){
			if(html.endsWith("\n"))
				html += "\n";
			html = html.replaceAll("\n", "<br>");
		} else {
			// Add mentions from the message
			for(const m of message.mentions){
				const m_sstr = message.text.substring(m.begin_i, m.end_i + 1);
				html = html.replace(m_sstr, `<span style="background: var(--clr_bg_selected)">${m_sstr}</span>`);
			}
		}
		return overlay ? [html, Markdown.marked_overlay.links] : html;
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
