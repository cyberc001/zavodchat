import {IDCache} from "$lib/cache/id.svelte.js";
import {RBTree} from "bintrees";

class RangeObserver {
	data = $state([]);
	loaded = $state(false);
	init_length = 0;
	start = 0; end = 0;
	
	cache;
	load_func;

	constructor(range, start, end, cache, load_func){
		this.start = start; this.end = end;
		this.init_length = end - start;
		for(let i = start; i < end; ++i)
			if(range.data[i - range.start])
				this.data[i - start] = range.data[i - range.start];

		this.cache = cache;
		this.load_func = load_func;
	}

	update(range, start, end){
		for(let i = start; i < end; ++i)
			if(range.data[i - range.start])
				this.data[i - this.start] = range.data[i - range.start];
		this.loaded = true;
	}

	contains(idx){
		return idx >= this.start && idx <= this.end;
	}
}

class DataRange {
	data = [];
	observers = [];

	start = 0; end = 0;
	t = 0; // creation order, latest range's values are prioritized when merging

	static _last_t = 0;
	update_t(){
		this.t = ++DataRange._last_t;
	}

	constructor(start, end){
		this.start = start; this.end = end;
		this.update_t();
	}

	latest_item(other, i){
		let f_empty = 0;
		if(typeof this.data[i - this.start] === "undefined")
			f_empty |= 1;
		if(typeof other.data[i - this.start] === "undefined")
			f_empty |= 2;
		if(f_empty == 0) // Objects are present for both ranges, so pick latest one
			return this.t > other.t ? this.data[i - this.start] : other.data[i - other.start];
		return f_empty == 1 ? other.data[i - other.start] : this.data[i - this.start];
	}

	keyed_start_end(key){
		if(typeof key === "undefined")
			return [this.start, this.end];
		const keys = [this.data[0][key], this.data[this.data.length - 1][key]];
		return keys[0] < keys[1] ? [keys[0], keys[1]] : [keys[1], keys[0]];
	}

	intersects(other, key){
		const [start, end] = this.keyed_start_end(key);
		const [o_start, o_end] = other.keyed_start_end(key);
		return (start >= o_start && start <= o_end)
		    || (start <= o_end && end >= o_start);
	}

	contains(other, key){
		const [start, end] = this.keyed_start_end(key);
		if(typeof other === "number")
			return other >= start && other <= end;
		const [o_start, o_end] = other.keyed_start_end(key);
		return o_start >= start && o_end <= end;
	}

	union(other){
		let new_range = new DataRange(Math.min(this.start, other.start), Math.max(this.end, other.end));
		new_range.tree = this.tree;

		// Solo beginning
		if(this.start < other.start)
			for(let i = 0; i < other.start - this.start; ++i)
				new_range.data[i] = this.data[i];
		else
			for(let i = 0; i < this.start - other.start; ++i)
				new_range.data[i] = other.data[i];
		// Intersection
		for(let i = Math.max(this.start, other.start); i < Math.min(this.end, other.end); ++i)
			new_range.data[i - new_range.start] = this.latest_item(other, i);
		// Solo end
		if(this.end > other.end)
			for(let i = other.end; i < this.end; ++i)
				new_range.data[i - new_range.start] = this.data[i - this.start];
		else
			for(let i = this.end; i < other.end; ++i)
				new_range.data[i - new_range.start] = other.data[i - other.start];


		new_range.observers = this.observers.slice();
		new_range.observers.push.apply(new_range.observers, other.observers);

		return new_range;
	}


	get_observers(start, end){
		if(typeof start === "undefined"){
			start = this.start;
			end = this.end;
		}

		let iter = {};
		let range = this;
		iter[Symbol.iterator] = function *(){
			for(let i = 0; i < range.observers.length; ++i){
				const obs = range.observers[i].deref();
				if(typeof obs === "undefined"){
					range.observers.splice(i--, 1);
					continue;
				}
				if(!range.intersects(new DataRange(obs.start, obs.end)))
					continue;
				yield obs;
			}
		}
		return iter;
	}

	update_observers(start, end, dont_fill){
		if(typeof start === "undefined"){
			start = this.start;
			end = this.end;
		}

		for(const obs of this.get_observers(start, end)){
			if(obs.end > this.end){
				// Try to load more items
				obs.loaded = false;
				obs.load_func(obs.cache, this, obs.start, obs.end - obs.start);
			} else if(!dont_fill)
				obs.update(this, start, end);
		}
	}

	reload_observers(){
		for(const obs of this.get_observers())
			obs.load_func(obs.cache, this, obs.start, obs.init_length);
	}

	remove(idx){
		for(const obs of this.get_observers())
			if(obs.contains(idx))
				obs.data.splice(idx - obs.start, 1);
		this.data.splice(idx - this.start, 1);
		--this.end;
		this.update_observers(this.start, this.end, true);
	}


	trim(){
		let start, end;
		for(const obs of this.get_observers()){
			const is_init = typeof start !== "undefined";
			if(!is_init || obs.start < start)
				start = obs.start;
			if(!is_init || obs.end > end)
				end = obs.end;
		}
		if(typeof start === "undefined" || this.start === this.end)
			return;
		this.data.splice(0, start - this.start);
		this.data.splice(end - this.end, this.end - end);
		this.start = start; this.end = end;
	}
}

class DataRangeTree {
	_tree;
	_max_idx = -1;

	constructor(){
		this._tree = new RBTree((a, b) => {
			return a.end - b.end;
		});
	}

	find_enclosing_range(range, key, node){
		if(typeof node === "undefined") // user call
			return this.find_enclosing_range(range, key, this._tree._root);

		if(node === null)
			return;
		if(node.data.contains(range, key))
			return node.data;

		const [start, end] = range.keyed_start_end(key);
		const [n_start, n_end] = node.data.keyed_start_end(key);
		if(end > n_end)
			return this.find_enclosing_range(range, key, node.right);
		return this.find_enclosing_range(range, key, node.left);
	}

	_by_id(id){
		const r = new DataRange(0, 0);
		r.data[0] = {id};
		const enc_range = this.find_enclosing_range(r, "id");
		if(!enc_range)
			return [undefined, undefined];
		for(let i = enc_range.start; i != enc_range.end; ++i)
			if(enc_range.data[i - enc_range.start].id === id)
				return [enc_range.data[i - enc_range.start], i];
		return [undefined, undefined];
	}
	find_by_id(id){
		const [data, _] = this._by_id(id);
		return data;
	}
	idx_by_id(id){
		const [_, idx] = this._by_id(id);
		return idx;
	}

	insert(range){
		range.tree = this;

		if(this._tree.size === 0){
			this._tree.insert(range);
			return range;
		}

		const iter = this._tree.upperBound(range); // find closest ancestor that has bigger end
		if(!iter.data())
			iter.prev();

		let removed_ranges = [];
		// ascend from lower bound until iter.data does not intersect range
		for(let r = iter.data(); r !== null && r.end >= range.start; r = iter.prev())
			if(range.intersects(r)){
				removed_ranges.push(r);
				range = range.union(r);
			}

		for(const r of removed_ranges)
			this._tree.remove(r);
		this._tree.insert(range);
		return range;
	}


	data_append(data, node, range){
		if(typeof range === "undefined"){ // user call
			let enc_range = this.find_enclosing_range(new DataRange(0, 0));
			if(enc_range){
				++enc_range.end;
				enc_range.data.unshift(data);
				for(const obs of enc_range.get_observers(0, 0)){
					if(obs.data.length < obs.init_length){
						++obs.end;
						obs.data.unshift(data);
					} else
						obs.update(this, this.start, this.end);
				}
				this.data_append(undefined, this._tree._root, enc_range);
			}
			if(this._max_idx !== -1)
				++this._max_idx;
			return;
		}

		// Shift all other ranges by 1
		if(node === null || node.data === range)
			return;

		++node.data.start; ++node.data.end;

		this.data_append(undefined, node.left, range);
		this.data_append(undefined, node.right, range);
	}

	data_remove(idx, node){
		if(typeof node === "undefined"){ // user call
			this.data_remove(idx, this._tree._root);
			if(this._max_idx !== -1)
				--this._max_idx;
			return;
		}

		if(node === null)
			return;

		// Shift all ranges past idx by 1
		if(node.data.contains(idx)){
			node.data.remove(idx);
		} else if(node.data.end > idx) {
			--node.data.start; --node.data.end;
		}

		if(node.data.end > idx) // potential ranges with (end > idx) in left subtree
			this.data_remove(idx, node.left);
		this.data_remove(idx, node.right);
	}
	data_remove_id(id){
		const idx = this.idx_by_id(id);
		if(typeof idx !== "undefined")
			this.data_remove(idx);
	}

	data_update(idx, data){
		let enc_range = this.find_enclosing_range(new DataRange(idx, idx));
		if(enc_range){
			for(const key in data)
				enc_range.data[idx - enc_range.start][key] = data[key];
			enc_range.update_observers(idx, idx + 1);
		}
	}
	data_update_id(id, data){
		const idx = this.idx_by_id(id);
		if(typeof idx !== "undefined")
			this.data_update(idx, data);
	}

	reload(node){
		if(typeof node === "undefined"){
			this.reload(this._tree._root);
			return;
		}
		if(node === null)
			return;
		node.data.reload_observers();
		this.reload(node.left);
		this.reload(node.right);
	}


	trim(node, toremove){
		if(typeof node === "undefined"){ // user call
			let toremove = [];
			this.trim(this._tree._root, toremove);
			for(const node of toremove)
				this._tree.remove(node);
			return;
		}
		if(node === null)
			return;

		const left = node.left, right = node.right;

		if(node.data.observers.length === 0)
			toremove.push(node.data);
		else
			node.data.trim();

		this.trim(left, toremove);
		this.trim(right, toremove);
	}
}

export class RangeCache extends IDCache {
	_default_state_constructor(){
		return new DataRangeTree();
	}

	intv = setInterval(() => {
		for(const id in this.cache)
			this.cache[id].trim();
	}, 60000);

	get_tree(_id){
		const id = this.state_refs_id(_id);
		return this.cache[id];
	}
	
	data_append(_id, data){
		const tree = this.get_tree(_id);
		if(tree)
			tree.data_append(data);
	}
	data_remove_id(_id, data_id){
		const tree = this.get_tree(_id);
		if(tree)
			tree.data_remove_id(data_id);
	}
	data_update_id(_id, data_id, data){
		const tree = this.get_tree(_id);
		if(tree)
			tree.data_update_id(data_id, data);
	}
	reload(_id){
		const tree = this.get_tree(_id);
		if(tree)
			tree.reload();
	}


	get_state(_id, start, count, load_func){
		const id = this.state_refs_id(_id);

		if(typeof this.cache[id] === "undefined")
			this.cache[id] = this._default_state_constructor();
		let tree = this.cache[id];

		if(tree._max_idx !== -1 && start + count > tree._max_idx)
			count = Math.max(0, tree._max_idx - start);

		const range = new DataRange(start, start + count);
		let enc_range = tree.find_enclosing_range(range);
		let load = false;
		if(!enc_range){
			enc_range = tree.insert(range);
			load = true;
		}

		let nobs = new RangeObserver(enc_range, start, start + count,
						this, load_func);
		nobs.loaded = !load;
		enc_range.observers.push(new WeakRef(nobs));

		if(load)
			load_func(this, enc_range, start, count);

		return nobs;	
	}
	// Should be called from get_state(, load_func), therefore id is not parsed twice
	set_state(range, start, count, data){
		if(range.end < start + data.length)
			range = range.tree.insert(new DataRange(start, start + data.length));
		if(range.end > start + data.length){
			range.end = start + data.length;
			// Remove excessive "undefined" items
			if(range.start + range.data.length > range.end)
				range.data.splice(range.end, range.start + range.data.length - range.end);
		}

		for(let i = start; i < start + count && i - start < data.length; ++i)
			range.data[i - range.start] = data[i - start];

		if(count > data.length && start + data.length <= range.end){
			range.tree._max_idx = start + data.length;
			for(const obs of range.get_observers()){
				if(obs.end > range.end)
					obs.end = range.end;
			}
		}

		range.update_observers(start, Math.min(start + count, range.end));
		range.update_t();
	}
}
