import IDCache from "$lib/cache/id.svelte.js";
import { RBTree } from "bintrees";

class RangeObserver {
	data = $state([]);
	start = 0;
	end = 0;

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

class DataRange {
	arr = [];
	observers = [];

	start = 0;
	end = 0;
	t = 0; // creation time, latest range's values are prioritized when merging

	constructor(start, end, init){
		if(typeof end === "undefined" && typeof init === "undefined"){
			// copy constructor
			let other = start;
			this.start = other.start;
			this.end = other.end;
			this.t = other.t;
			for(let i = 0; i < this.end - this.start; ++i)
				this.arr.push(other.arr[i]);
			return;
		}

		this.start = start;
		this.end = end;
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
	contains(other){
		if(typeof other === "number")
			return other >= this.start && other <= this.end;
		return this.start <= other.start && this.end >= other.start;
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
		
		new_range.observers = this.observers;
		new_range.observers.push.apply(new_range.observers, other.observers);

		return new_range;
	}

	update_observers(start, end, missing){
		if(typeof start === "undefined"){
			start = this.start;
			end = this.end;
		}

		// Thanks JavaScript developers for making WeakMap non-iterable, Lua doesnt totally have a WeakMap that works for my purposes https://stackoverflow.com/questions/36760314/how-to-make-weak-set-or-weak-map-iterable-in-es6
		let new_obs = [];
		for(const ref of this.observers){
			const obs = ref.deref();
			if(obs)
				new_obs.push(ref);
		}
		this.observers = new_obs;

		for(const ref of this.observers){
			const obs = ref.deref();
			if(obs){ // in case an object gets garbage collected after new_obs is formed
				if(missing && obs.end > this.end){
					let m = obs.end - this.end;
					obs.end = this.end;
					obs.data.splice(obs.data.length - m, m);
				}
				obs.update(this, start, end);
			}
		}
	}
};

export default class RangeCache extends IDCache {
	cache = {};

	// RB-tree methods
	_default_state_constructor(){
		let tree = new RBTree((a, b) => {
			return a.end - b.end;
			//return a.start - b.start;
		});

		return tree;
	}

	_find_enclosing_range(tree, range){
		const iter = tree.lowerBound(range);
		if(!iter?._cursor || !iter.data()?.contains(range))
			return null;
		return iter.data();
	}

	_insert_range(tree, range){
		if(tree.size === 0){
			tree.insert(range);
			return range;
		}

		const iter = tree.upperBound(range); // find closest ancestor that has bigger end
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
		tree.insert(range);
		for(const r of deleted_subranges)
			tree.remove(r);
		return range;
	}

	// remove a single element at index idx, subtracting 1 from the range this index is in, and shifting left by 1 all ranges with bigger end
	_remove_one(tree, idx){
		let range = new DataRange(idx, idx);

		const iter = tree.lowerBound(range);
		let containing_range = iter.data();
		if(!containing_range || !containing_range.contains(range)){
			console.error(`Attempt to remove index ${idx} from tree that does not contain it`, JSON.parse(JSON.stringify(tree)));
			return;
		}

		containing_range.arr.splice(idx - containing_range.start, 1);
		--containing_range.end;
		this.__r_remove_one_iter(tree._root, idx);
	}
	__r_remove_one_iter(node, idx){
		if(node === null)
			return;
		if(!node.data.contains(idx) && node.data.end > idx){
			--node.data.start; --node.data.end;
		}

		if(node.data.end > idx){
			// potential ranges with (end > idx) in left subtree
			this.__r_remove_one_iter(node.left, idx);
		}
		this.__r_remove_one_iter(node.right, idx);
	}


	get_state(_id, start, count, load_func){
		let tree = this._default_state_constructor();

		let id = this.state_refs_id(_id);
		let range = new DataRange(start, start + count);

		if(typeof this.cache[id] === "undefined")
			this.cache[id] = this._default_state_constructor();

		let enc_range = this._find_enclosing_range(this.cache[id], range);
		let load = false;
		if(!enc_range){
			enc_range = this._insert_range(this.cache[id], range);
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
		for(let i = start; i < start + count && i - start < data.length; ++i)
			range.arr[i - range.start] = data[i - start];
	
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
