export default class Select {
	static __get_selection_index(el, range){
		if(el === range.startContainer){
			if(el.nodeType === Node.TEXT_NODE)
				return [true, range.startOffset];

			let idx = 0;
			for(let i = 0; i < el.childNodes.length && i < range.startOffset; ++i)
				idx += Select.get_inner_text(el.childNodes[i]).length;
			return [true, idx];
		}
		let i = 0;
		for(const child of el.childNodes){
			const [found, adv] = Select.__get_selection_index(child, range);
			if(found){
				// If double-faced-element is incorrect, set caret to it's start
				const de_result = Select.get_element_type(el);
				return [true, i + (de_result[0] === Select.ElementTypes.DoubleFacedInvalid ? 0 : adv)];
			}
			i += adv;
		}
		return [false, Select.get_inner_text(el).length];
	}
	static get_selection_index(el, raw){
		const range = window.getSelection().getRangeAt(0);
		if(!range.intersectsNode(el))
			return;

		const [found, adv] = Select.__get_selection_index(el, range, raw);
		if(found)
			return adv;
	}

	static __set_selection_index(el, idx){
		const ln = Select.get_inner_text(el).length;

		console.log("__set_selection_index", el, idx, ln);
		const de_result = Select.get_element_type(el);
		const is_double_faced = de_result[0] === Select.ElementTypes.DoubleFaced ||
					de_result[0] === Select.ElementTypes.DoubleFacedSolid;
		if(idx < ln){
			console.log("type", de_result[0]);
			if(!el.childNodes.length || is_double_faced){
				window.getSelection().removeAllRanges();
				const range = document.createRange();
				if(el.nodeType === Node.TEXT_NODE)
					range.setStart(el, idx);
				else if(is_double_faced){
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
		if(!is_double_faced)
			for(const child of el.childNodes){
				if(Select.__set_selection_index(child, idx - i))
					return true;
				i += Select.get_inner_text(child).length;
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
	// If they have dataExpectedText their innerText doesn't contain it as a substring, they get erased completely.

	static ElementTypes = {
		Normal: 0,
		DoubleFacedInvalid: 1,
		DoubleFaced: 2,
		DoubleFacedSolid: 3 // does not have expectedText - a single, solid element
	};

	static get_element_type(el){
		if(!el.dataset?.rawText)
			return [Select.ElementTypes.Normal];
		if(typeof(el.dataset?.expectedText) === "undefined")
			return [Select.ElementTypes.DoubleFacedSolid];

		let child_text = "";
		for(const child of el.childNodes)
			child_text += Select.get_inner_text(child);
		// Check expectedText
		const idx = child_text.indexOf(el.dataset.expectedText);
		if(idx > -1)
			return [Select.ElementTypes.DoubleFaced, child_text, idx];
		return [Select.ElementTypes.DoubleFacedInvalid];
	}
	static is_in_double_faced_element(el){
		while(el.parentNode){
			if(el.dataset?.rawText)
				return true;
			el = el.parentNode;
		}
		return false;
	}

	static get_inner_text(el){
		let text = "";
		if(el.dataset?.rawText){
			const de_result = Select.get_element_type(el);
			// Replace the substring in innerText with data-raw-text, if the element is valid
			// Otherwise, text === "" and it gets erased
			if(de_result[0] !== Select.ElementTypes.DoubleFacedInvalid)
				text = de_result[0] === Select.ElementTypes.DoubleFacedSolid ? el.dataset.rawText :
					de_result[1].substring(0, de_result[2]) +
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


	// Utils
	static get_coords(el){
		const range = window.getSelection().getRangeAt(0);
		if(!range.intersectsNode(el))
			return;

		// https://stackoverflow.com/questions/6846230/coordinates-of-selected-text-in-browser-page
		const span = document.createElement("span");
		span.appendChild(document.createTextNode("\u200b"));
		range.insertNode(span);
		
		const el_rect = el.getBoundingClientRect();
		const rect = span.getBoundingClientRect();
		const coords = [rect.left - el_rect.left, rect.top - el_rect.top];

		const _parent = span.parentNode;
		_parent.removeChild(span);
		_parent.normalize();

		return coords;
	}
	static clone_range(range){
		return {
			startContainer: range.startContainer, startOffset: range.startOffset,
			endContainer: range.endContainer, endOffset: range.endOffset
		};
	}
};
