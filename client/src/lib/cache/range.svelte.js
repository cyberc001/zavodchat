import {IDCache} from '$lib/cache/id.svelte.js';
import {RBTree} from 'bintrees';
import Util from '$lib/util.js';

class RangeObserver {
	data = $state([]);
	loaded = $state(false);
	start_id = 0; count = 0;
	asc; asc_items;
	order_sign = $derived(this.asc ? 1 : -1);

	is_full = $derived(this.data.length >= this.count);
	
	tree; load_func;

	constructor(tree, start_id, count, load_func, asc, asc_items){
		this.start_id = start_id;
		this.count = count;
		this.asc = asc; this.asc_items = asc_items;

		this.set(tree, start_id, count);

		this.tree = tree;
		this.load_func = load_func;
	}
	clone(){
		const obs = new RangeObserver(this.tree, this.start_id, this.count, this.load_func, this.asc, this.asc_items);
		this.tree.observers.push(new WeakRef(obs));
		return obs;
	}

	find_closest_start_iter(tree){
		if(this.asc)
			return tree._tree.lowerBound({id: this.start_id});
		else {
			const iter = tree._tree.lowerBound({id: this.start_id});
			if(!iter.data())
				iter.prev();
			while(iter.data()){
				if(iter.data().id <= this.start_id)
					return iter;
				iter.prev();
			}
		}
	}

	set(tree, start_id, count){
		let iter = this.find_closest_start_iter(tree);
		if(!iter)
			return;

		const inv_items = typeof this.asc_items !== "undefined" && this.asc_items !== this.asc;
		let i = inv_items ? this.count - 1 : 0;
		while(iter.data() && i < this.count && i >= 0){
			this.data[inv_items ? i-- : i++] = iter.data();
			if(this.asc)
				iter.next();
			else
				iter.prev();
		}
		if(!iter.data() && inv_items)
			this.data.splice(0, i + 1);

		this.loaded = true;
	}

	find(id){
		return Util.bin_search(this.data, (x) => (x.id - id) * this.order_sign);
	}
	remove(id){
		const i = this.find(id);
		if(i !== -1)
			this.data.splice(i, 1);
		// Try to load more
		this.loaded = false;
		this.load_func(this.tree, this.start_id, this.count, this.asc);
	}

	get min_id(){
		return Math.min(this.data[0].id, this.data[this.data.length - 1].id);
	}
	get max_id(){
		return Math.max(this.data[0].id, this.data[this.data.length - 1].id);
	}

	get end_id(){
		return this.data.length === 0 ? (this.asc ? RangeCache.max_id : 0)
							: (this.asc ? this.max_id : this.min_id);
	}

	intersects(start_id, end_id){
		let this_end_id = this.end_id;
		let a = Math.min(this.start_id, this_end_id), b = Math.max(this.start_id, this_end_id);
		return (start_id >= a && start_id <= b) || (end_id >= a && end_id <= b);
	}
	intersects_items(other){
		if(this.data.length === 0 || other.data.length === 0)
			return false;
		return (other.min_id >= this.min_id && other.min_id <= this.max_id)
			|| (other.max_id >= this.min_id && other.max_id <= this.max_id);
	}

	has(id){
		if(this.data.length === 0)
			return false;
		return id >= this.min_id && id <= this.max_id;
	}
}

class DataTree {
	_tree;
	observers = [];
	min_id = RangeCache.max_id; max_id = -1;

	get_observers(start_id, end_id, or_nonfull){
		let iter = {};
		let tree = this;
		iter[Symbol.iterator] = function *(){
			for(let i = 0; i < tree.observers.length; ++i){
				const obs = tree.observers[i].deref();
				if(typeof obs === "undefined"){
					tree.observers.splice(i--, 1);
					continue;
				}

				if(typeof start_id !== "undefined" && !obs.intersects(start_id, end_id) && (!or_nonfull || obs.is_full))
					continue;

				yield obs;
			}
		}
		return iter;
	}

	constructor(){
		this._tree = new RBTree((a, b) => {
			return a.id - b.id;
		});
	}

	// For loading
	has_enough(obs){
		if(obs.data.length >= obs.count)
			return true;

		if(obs.data.length === 0)
			return obs.asc ? this.min_id !== -1 && obs.start_id >= this.min_id
				: this.max_id !== RangeCache.max_id && obs.end_id <= this.max_id;

		// Observer doesnt have all requested data
		if(obs.asc){
			let end = this._tree.lowerBound({id: obs.max_id});
			if(typeof end.data().next_id === "undefined" && (this.max_id === -1 || end.data().next_id <= this.max_id))
				return false;
			if(typeof end.data().next_id !== "undefined" && !this._tree.find({id: end.data().next_id}))
				return false;
		} else {
			let beg = this._tree.lowerBound({id: obs.min_id});
			if(typeof beg.data().prev_id === "undefined" && (this.min_id === RangeCache.max_id || beg.data().prev_id >= this.min_id))
				return false;
			if(typeof beg.data().prev_id !== "undefined" && !this._tree.find({id: beg.data().prev_id}))
				return false;
		}
		return true;

		/*if(obs.asc)
			return this.min_id !== -1 && obs.start_id >= this.min_id;
		return this.max_id !== RangeCache.max_id && obs.end_id <= this.max_id;*/
	}
	set_state(start_id, count, data, asc){
		for(let i = 0; i < data.length; ++i){
			let f = this._tree.find(data[i]);
			if(!f){
				this._tree.insert(data[i]);
				f = data[i];
			}
			if(asc){
				if(i > 0)
					f.prev_id = data[i - 1].id;
				if(i < data.length - 1)
					f.next_id = data[i + 1].id;
			} else {
				if(i > 0)
					f.next_id = data[i - 1].id;
				if(i < data.length - 1)
					f.prev_id = data[i + 1].id;
			}
		}
		if(data.length > 0 && count > data.length){
			if(asc){
				if(data[data.length - 1].id < this.min_id)
					this.min_id = data[data.length - 1].id;
			} else {
				if(data[data.length - 1].id > this.max_id)
					this.max_id = data[data.length - 1].id;
			}
		}
		const end_id = data.length > 0 ? data[data.length - 1].id : start_id;
		for(const obs of this.get_observers(start_id, end_id))
			obs.set(this, start_id, count);
	}

	// Update next and previous elements, similar to linked lists
	update_neighbours(data, next, prev){
		let iter = this._tree.lowerBound(data);
		iter.next();
		if(iter.data())
			next(iter.data());
		iter = this._tree.lowerBound(data);
		iter.prev();
		if(iter.data())
			prev(iter.data());
	}

	insert(data, update){
		this._tree.insert(data);

		if(update){
			this.update_neighbours(data,
						(next) => next.prev_id = data.id,
						(prev) => prev.next_id = data.id);
			for(const obs of this.get_observers(data.id, data.id, true)){
				delete obs.adjusted; // TODO thats a hack
				obs.last_action = "inserted";
				obs.set(this, data.id, obs.count);
			}
		}
	}
	remove(id, update){
		let data = this._tree.find({id});

		if(update){
			if(data)
				this.update_neighbours(data,
							(next) => next.prev_id = data.prev_id,
							(prev) => prev.next_id = data.next_id);
			for(const obs of this.get_observers(id, 1)){
				obs.last_action = "removed";
				obs.remove(id);
			}
		}

		this._tree.remove({id});
	}
	update(data){
		let d = this._tree.find(data);
		if(d)
			for(const key of Object.keys(data))
				d[key] = data[key];

		for(const obs of this.get_observers(data.id, data.id)){
			obs.last_action = "updated";
			obs.set(this, data.id, 1);
		}
	}

	reload(){
		for(const obs of this.get_observers())
			obs.load_func(this, obs.start_id, obs.count, obs.asc);
	}

	// This function runs in O(NM) + O(KlogN), where N - tree size, M - observer count, K - deleted entries count
	// Seems kinda bad, but realistically N, M and K are rather low, cause only so much entries can be loaded via HTTP in-between trims
	// TODO trim when tree size gets too big
	// TODO delete observers from ranges when iter got past them
	trim(){
		let ranges = [];
		for(const obs of this.get_observers()){
			if(obs.loading)
				continue;
			ranges.push(obs);
		}

		let todelete = [];
		let iter = this._tree.iterator();
		iter.next();
		while(iter.data()){
			let data = iter.data();
			let can_delete = true;
			for(const obs of ranges){
				if(obs.has(data.id)){
					can_delete = false;
					break;
				}
			}
			if(can_delete)
				todelete.push(data);

			iter.next();
		}

		for(const data of todelete)
			this._tree.remove(data);
	}
}

export class RangeCache extends IDCache {
	static max_id = 2147483647;

	_default_state_constructor(){
		return new DataTree();
	}

	intv = setInterval(() => {
		for(const id in this.cache)
			this.cache[id].trim();
	}, 15000/*60000*/);

	get_tree(_id){
		const id = this.state_refs_id(_id);
		return this.cache[id];
	}
	
	insert(_id, data){
		const tree = this.get_tree(_id);
		if(tree)
			tree.insert(data, true);
	}
	remove(_id, data_id){
		const tree = this.get_tree(_id);
		if(tree)
			tree.remove(data_id, true);
	}
	update(_id, data){
		const tree = this.get_tree(_id);
		if(tree)
			tree.update(data);
	}

	reload(_id){
		const tree = this.get_tree(_id);
		if(tree)
			tree.reload();
	}


	get_state(_id, start_id, count, load_func, asc, asc_items){
		const id = this.state_refs_id(_id);

		if(typeof this.cache[id] === "undefined")
			this.cache[id] = this._default_state_constructor();
		let tree = this.cache[id];

		let nobs = new RangeObserver(tree, start_id, count, load_func, !!asc, asc_items);
		nobs.loaded = tree.has_enough(nobs);
		tree.observers.push(new WeakRef(nobs));

		if(!nobs.loaded)
			load_func(tree, start_id, count, asc);

		return nobs;
	}
}
