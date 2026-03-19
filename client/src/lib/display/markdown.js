import {Marked} from 'marked';
import {markedHighlight} from "marked-highlight";
import hljs from 'highlight.js';

export default class Markdown {
	static marked = new Marked(
		markedHighlight({
			emptyLangClass: 'hljs',
			langPrefix: 'hljs language-',
			highlight(code, lang, info) {
				console.log("HIGLIGHT");
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
				const [m1, m2] = Markdown.__get_md_markers(token);
				return `<code>${m1}${token.text}${m2}</code>`;
			}
		},

		walkTokens(token){
			if(token.type === "link")
				this.links.push(token.text);
		}
	});

	static parse(text, overlay){
		Markdown.marked_overlay.links = [];
		let html = overlay ? Markdown.marked_overlay.parseInline(text) : Markdown.marked.parse(text);
		if(overlay){
			if(html.endsWith("\n"))
				html += "\n";
			html = html.replaceAll("\n", "<br>");
		}
		return overlay ? [html, Markdown.marked_overlay.links] : html;
	}
};

Markdown.marked.use({renderer: {
	image(token){
		return token.raw;
	},
	blockquote(token){
		return `<blockquote>${this.parser.parse(token.tokens)}</blockquote>`;
	}
}});
