export default class Select {
	static __get_total_text_ln(el){
		if(!el.childNodes.length){
			switch(el.nodeName){
				case "BR":
					return 1;
			}
			return typeof el.innerText !== "undefined" ? el.innerText.length : el.textContent.length;
		}
		let sel_ln = 0;
		for(const child of el.childNodes)
			sel_ln += Select.__get_total_text_ln(child);
		return sel_ln;
	}

	static __get_selection_index(el, range){
		if(el === range.startContainer){
			if(el.nodeType === Node.TEXT_NODE)
				return [true, range.startOffset];

			let idx = 0;
			for(let i = 0; i < el.childNodes.length && i < range.startOffset; ++i)
				idx += Select.__get_total_text_ln(el.childNodes[i]);
			return [true, idx];
		}
		let i = 0;
		for(const child of el.childNodes){
			const [found, adv] = Select.__get_selection_index(child, range);
			if(found)
				return [true, i + (Select.is_correct_display_element(el) === false ? 0 : adv)];
			i += adv;
		}
		return [false, Select.__get_total_text_ln(el)];
	}
	static get_selection_index(el){
		const range = window.getSelection().getRangeAt(0);
		if(!range.intersectsNode(el))
			return;

		const [found, adv] = Select.__get_selection_index(el, range);
		if(found)
			return adv;
	}

	static __set_selection_index(el, idx){
		const ln = Select.__get_total_text_ln(el);

		if(!el.childNodes.length){
			if(idx < ln){
				window.getSelection().removeAllRanges();
				const range = document.createRange();
				if(el.nodeType === Node.TEXT_NODE)
					range.setStart(el, idx);
				else
					range.setStartBefore(el);
				window.getSelection().addRange(range);
				return true;
			}
		}

		let i = 0;
		for(const child of el.childNodes){
			if(Select.__set_selection_index(child, idx - i))
				return true;
			i += Select.__get_total_text_ln(child);
		}
		return false;
	}
	static set_selection_index(el, idx){
		if(!Select.__set_selection_index(el, idx)){
			window.getSelection().removeAllRanges();
			const range = document.createRange();
			const last_child = el.childNodes[el.childNodes.length - 1];
			if(!last_child){
				range.setStart(el, 0);
			} else {
				range.selectNode(last_child);
				range.collapse(false);
			}
			window.getSelection().addRange(range);
			el.focus();
		}
	}

	static is_correct_display_element(el){
		if(!el.dataset?.rawText)
			return true;
		let child_text = "";
		for(const child of el.childNodes)
			child_text += Select.get_inner_text(child);
		return child_text.startsWith(el.dataset.expectedText) ? child_text : false;
	}
	static get_inner_text(el){
		let text = "";
		if(el.dataset?.rawText){
			const child_text = Select.is_correct_display_element(el)
			if(child_text !== false)
				text = el.dataset.rawText + child_text.substring(el.dataset.expectedText.length);
		} else if(!el.childNodes.length)
			text = typeof el.innerText !== "undefined" ? el.innerText : el.textContent;
		else
			for(const child of el.childNodes)
				text += Select.get_inner_text(child);
		switch(el.nodeName){
			case "BR":
				text += "\n";
				break;
		}
		return text;
	}
};
