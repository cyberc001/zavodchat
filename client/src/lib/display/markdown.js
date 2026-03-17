import {marked} from 'marked';
import {markedHighlight} from "marked-highlight";
import hljs from 'highlight.js';

const renderer = {
	image(token){
		return token.raw;
	},
	blockquote(token){
		return `<blockquote>${this.parser.parse(token.tokens)}</blockquote>`;
	}
};

marked.use(
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
marked.use({renderer});

export default class Markdown {
	static parse(text){
		const html = marked.parse(text);
		return html;
	}
};

