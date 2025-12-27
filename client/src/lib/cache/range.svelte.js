import IDCache from "$lib/cache/id.svelte.js";
import { RBTree } from "bintrees";

class RangeObserver {
	data = $state([]);
	start = 0; end = 0;

	constructor(range, start, end){
		this.start = start;
		this.end = end;
		for(let i = start; i < end; ++i)
			this.data.push(range.arr[i - range.start]);
	}

	update(range, start, end){
		for(let i = start; i < end; ++i)
			this.data[i - this.start] = range.arr[i - range.start];
	}
}

export class DataRange {
	arr = [];
	observers = [];

	start = 0; end = 0;
	id_start = 0; id_end = 0;
	t = 0; // creation time, latest range's values are prioritized when merging

	constructor(start, end, init){
		// empty constructor; could be used for comparison ranges
		if(typeof start === "undefined")
			return;

		if(typeof end === "undefined" && typeof init === "undefined"){
			// copy constructor
			let other = start;
			this.start = other.start; this.end = other.end;
			this.id_start = other.id_start; this.id_end = other.id_end;
			this.t = other.t;
			for(let i = 0; i < this.end - this.start; ++i)
				this.arr.push(other.arr[i]);
			return;
		}

		this.start = start; this.end = end;
		this.t = performance.now();
		for(let i = 0; i < end - start; ++i)
			this.arr.push(init ? {data: start + i, t: this.t} : {});
	}

	latest_item(other, i){
		let flags = 0;
		if(Object.keys(this.arr[i - this.start]).length === 0)
			flags |= 0x1;
		if(Object.keys(other.arr[i - other.start]).length === 0)
			flags |= 0x2;
		if(flags == 0x0 || flags == 0x3)
			return this.t > other.t ? this.arr[i - this.start] : other.arr[i - other.start];
		return flags == 0x1 ? other.arr[i - other.start] : this.arr[i - this.start];
	}

	intersects(other){
		return (this.end >= other.start && this.start <= other.end)
		|| (this.start <= other.end && this.end >= other.start);
	}
	intersects_id(other){
		return (this.id_end >= other.id_start && this.id_start <= other.id_end)
		|| (this.id_start <= other.id_end && this.id_end >= other.id_start);
	}

	contains(other){
		if(typeof other === "number")
			return other >= this.start && other <= this.end;
		return this.start <= other.start && this.end >= other.start;
	}
	contains_id(other){
		if(typeof other === "number")
			return other >= this.id_start && other <= this.od_end;
		return this.id_start <= other.id_start && this.id_end >= other.id_start;
	}

	union(other){
		let new_range = new DataRange(Math.min(this.start, other.start), Math.max(this.end, other.end));

		// Solo beginning
		if(this.start < other.start)
			for(let i = 0; i < other.start - this.start; ++i)
				new_range.arr[i] = this.arr[i];
		else
			for(let i = 0; i < this.start - other.start; ++i)
				new_range.arr[i] = other.arr[i];
		// Intersection
		for(let i = Math.max(this.start, other.start); i < Math.min(this.end, other.end); ++i)
			new_range.arr[i - new_range.start] = this.latest_item(other, i);
		// Solo end
		if(this.end > other.end)
			for(let i = other.end; i < this.end; ++i)
				new_range.arr[i - new_range.start] = this.arr[i - this.start];
		else
			for(let i = this.end; i < other.end; ++i)
				new_range.arr[i - new_range.start] = other.arr[i - other.start];

		new_range.id_start = Math.min(this.id_start, other.id_start);
		new_range.id_end = Math.max(this.id_end, other.id_end);

		new_range.observers = this.observers;
		new_range.observers.push.apply(new_range.observers, other.observers);

		return new_range;
	}

	delete_gced_observers(){
		// Thanks JavaScript developers for making WeakMap non-iterable, Lua doesnt totally have a WeakMap that works for my purposes https://stackoverflow.com/questions/36760314/how-to-make-weak-set-or-weak-map-iterable-in-es6
		let new_obs = [];
		for(const ref of this.observers){
			const obs = ref.deref();
			if(obs)
				new_obs.push(ref);
		}
		this.observers = new_obs;
	}
	update_observers(start, end){
		if(typeof start === "undefined"){
			start = this.start;
			end = this.end;
		}

		this.delete_gced_observers();

		for(const ref of this.observers){
			const obs = ref.deref();
			if(obs){ // in case an object gets garbage collected after new_obs is formed
				if(obs.end > this.end){
					let m = obs.end - this.end;
					obs.end = this.end;
					obs.data.splice(obs.data.length - m, m);
				}
				obs.update(this, start, end);
			}
		}
	}
	inc_observers_end(){
		this.delete_gced_observers();

		for(const ref of this.observers){
			const obs = ref.deref();
			if(obs){
				++obs.end;
				obs.update(this, this.start, this.end);
			}
		}
	}
};

export class DataRangeTree {
	_tree;

	constructor(){
		this._tree = new RBTree((a, b) => {
			return a.end - b.end;
		});
	}

	find_enclosing_range(range){
		const iter = this._tree.lowerBound(range);
		if(!iter?._cursor || !iter.data()?.contains(range))
			return null;
		return iter.data();
	}
	find_enclosing_range_id(range){
		return this.__find_enclosing_range_id_iter(this._tree._root, range);
	}
	__find_enclosing_range_id_iter(node, range){
		if(node === null)
			return;
		if(node.data.contains_id(range))
			return node.data;

		if(range.id_end > node.data.id_end)
			return __find_enclosing_range_id_iter(node.right, range);
		return __find_enclosing_range_id_iter(node.left, range);
	}


	find_idx_of_id(id){
		const r = new DataRange();
		r.id_start = id;
		r.id_end = id;
		const enc_range = this.find_enclosing_range_id(r);
		if(!enc_range)
			return;
		for(let i = enc_range.start; i < enc_range.end; ++i)
			if(enc_range.arr[i - enc_range.start].id === id)
				return i;
	}
	find_by_id(id){
		const r = new DataRange();
		r.id_start = id;
		r.id_end = id;
		const enc_range = this.find_enclosing_range_id(r);
		if(!enc_range)
			return;
		for(let i = enc_range.start; i < enc_range.end; ++i)
			if(enc_range.arr[i - enc_range.start].id === id)
				return enc_range.arr[i - enc_range.start];
	}

	insert_range(range){
		if(this._tree.size === 0){
			this._tree.insert(range);
			return range;
		}

		const iter = this._tree.upperBound(range); // find closest ancestor that has bigger end
		if(!iter.data())
			iter.prev();

		let deleted_subranges = [];
		// ascend from lower bound until iter.data does not intersect range
		for(let r = iter.data();
			r !== null && r.end >= range.start;
			r = iter.prev()){
				if(range.intersects(r)){
					deleted_subranges.push(r);
					range = range.union(r);
				}
		}
		this._tree.insert(range);
		for(const r of deleted_subranges)
			this._tree.remove(r);
		return range;
	}


	insert_last(data){
		let range = new DataRange(0, 0);

		const iter = this._tree.lowerBound(range);
		let containing_range = iter.data();
		if(!containing_range.contains(range))
			containing_range = undefined;
		if(containing_range){
			++containing_range.end;
			if(data.id > containing_range.id_end)
				containing_range.id_end = data.id;
			containing_range.arr.unshift(data);
			containing_range.inc_observers_end();
			//containing_range.update_observers();
		}

		// shift everything right by 1 except the containing range
		this.__insert_last_iter(this._tree._root, containing_range);
	}
	__insert_last_iter(node, r){
		if(node === null || node.data === r)
			return;

		++node.data.start; ++node.data.end;
		node.data.update_observers();

		this.__insert_last_iter(node.left, r);
		this.__insert_last_iter(node.right, r);
	}

	// remove a single element at index idx, subtracting 1 from the range this index is in, and shifting left by 1 all ranges with bigger end
	remove_one(idx){
		let range = new DataRange(idx, idx);

		const iter = this._tree.lowerBound(range);
		let containing_range = iter.data();
		if(!containing_range || !containing_range.contains(range)){
			console.error(`Attempt to remove index ${idx} from tree that does not contain it`, JSON.parse(JSON.stringify(tree)));
			return;
		}

		containing_range.arr.splice(idx - containing_range.start, 1);
		--containing_range.end;
		containing_range.update_observers();
		this.__remove_one_iter(this._tree._root, idx);
	}
	__remove_one_iter(node, idx){
		if(node === null)
			return;

		if(!node.data.contains(idx) && node.data.end > idx){
			--node.data.start; --node.data.end;
			node.data.update_observers();
		}

		if(node.data.end > idx){
			// potential ranges with (end > idx) in left subtree
			this.__remove_one_iter(node.left, idx);
		}
		this.__remove_one_iter(node.right, idx);
	}
	remove_one_id(id){
		let idx = this.find_idx_of_id(id);
		if(typeof idx !== "undefined")
			this.remove_one(idx);
	}

	update_one(idx, data){
		let range = new DataRange(idx, idx);

		const iter = this._tree.lowerBound(range);
		let containing_range = iter.data();
		if(!containing_range || !containing_range.contains(range))
			return; // ignore the update

		for(const f in data)
			containing_range.arr[idx - containing_range.start][f] = data[f];
		containing_range.update_observers(idx, idx + 1);
	}
	update_one_id(id, data){
		let idx = this.find_idx_of_id(id);
		if(typeof idx !== "undefined")
			this.update_one(idx, data);
	}
};

export default class RangeCache extends IDCache {
	cache = {};

	// RB-tree methods
	_default_state_constructor(){
		return new DataRangeTree();
	}

	get_tree(_id){
		const id = this.state_refs_id(_id);
		return this.cache[id];
	}

	get_state(_id, start, count, load_func){
		let id = this.state_refs_id(_id);
		let range = new DataRange(start, start + count);

		if(typeof this.cache[id] === "undefined")
			this.cache[id] = this._default_state_constructor();
		let tree = this.cache[id];

		let enc_range = tree.find_enclosing_range(range);
		let load = false;
		if(!enc_range){
			enc_range = tree.insert_range(range);
			load = true;
		}

		let nobs = new RangeObserver(enc_range, start, start + count);
		enc_range.observers.push(new WeakRef(nobs));

		if(load)
			load_func(this, enc_range, start, count);

		return nobs;
	}
	// Should be called from get_state(, load_func), therefore id is not parsed twice
	set_state(range, start, count, data){
		let min_id = data[0]?.id, max_id = data[0]?.id;
		for(let i = start; i < start + count && i - start < data.length; ++i){
			let dat = data[i - start];
			range.arr[i - range.start] = dat;
			if(dat.id < min_id)
				min_id = dat.id;
			if(dat.id > max_id)
				max_id = dat.id;
		}
		if(range.id_start > min_id)
			range.id_start = min_id;
		if(range.id_end < max_id)
			range.id_end = max_id;
	
		let missing = count - data.length;
		// If we were loading the end of range and server underdelivered, remove dummy items
		if(missing > 0 && start + count === range.end){
			range.end -= missing;
			range.arr.splice(range.arr.length - missing, missing);
		}
		range.update_observers(start, Math.min(start + count, range.end), missing > 0);

		range.t = performance.now();
	}
}
