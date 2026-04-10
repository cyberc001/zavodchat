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
			if(found){
				// If double-faced-element is incorrect, set caret to it's start
				const de_result = Select.is_correct_double_faced_element(el);
				return [true, i + (de_result[0] ? adv : 0)];
			}
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

		if(idx < ln){
			const de_result = Select.is_correct_double_faced_element(el);
			if(!el.childNodes.length || typeof(de_result[1]) !== "undefined"){
				window.getSelection().removeAllRanges();
				const range = document.createRange();
				if(el.nodeType === Node.TEXT_NODE)
					range.setStart(el, idx);
				else if(typeof(de_result[1]) !== "undefined"){
					// If caret is at the beginning (i.e. user typed a character before a double-faced element),
					// collapse selection to beginning; otherwise (double-faced element just appeared), collapse to end
					range.selectNode(el);
					range.collapse(idx === 0);
				} else
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
			// Fallback: set focus on last child (so it stays in the div and user can still type)
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


	// Double-faced elements:
	// Their content evaluates to data-raw-text in get_inner_text().
	// If their innerText doesn't contain data-expected-text as a substring, they get erased completely.

	// Checks if this is just a normal element, or a double-faced one.
	// For double-faced elements, checks that its data-expected-text is equal to it's inner text.
	static is_correct_double_faced_element(el){
		if(!el.dataset?.rawText)
			return [true];
		let child_text = "";
		for(const child of el.childNodes)
			child_text += Select.get_inner_text(child);
		const idx = child_text.indexOf(el.dataset.expectedText);
		if(idx > -1)
			return [true, child_text, idx];
		return [false];
	}
	static get_inner_text(el){
		let text = "";
		if(el.dataset?.rawText){
			const de_result = Select.is_correct_double_faced_element(el);
			// Replace the substring in innerText with data-raw-text, if the element is correct
			// Otherwise, text === "" and it gets erased
			if(de_result[1])
				text = de_result[1].substring(0, de_result[2]) +
					el.dataset.rawText +
					de_result[1].substring(de_result[2] + el.dataset.expectedText.length);
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
