import {IDCache, IDObserver} from '$lib/cache/id.svelte.js';
import {RBTree} from 'bintrees';
import Util from '$lib/util.js';

class RangeObserver extends IDObserver {
	data = $state([]);
	start_id = 0; count = 0;
	asc; asc_items;

	order_sign = $derived(this.asc ? 1 : -1);
	inv_items = $derived(typeof this.asc_items !== "undefined" && this.asc_items !== this.asc);

	is_full = $derived(this.data.length >= this.count);
	
	tree;

	load_func;
	onupdate; onremove; oninsert;

	constructor(tree, start_id, count, load_func, asc, asc_items){
		super();
		this.start_id = start_id;
		this.count = count;
		this.asc = asc; this.asc_items = asc_items;

		this.set(tree, start_id, count);

		this.tree = tree;
		this.load_func = load_func;
	}
	destroy(){
		if(this.tree)
			this.tree.observers.splice(this.tree.observers.findIndex((x) => x.deref() === this), 1);
	}

	clone(){
		const obs = new RangeObserver(this.tree, this.start_id, this.count, this.load_func, this.asc, this.asc_items);
		obs.onupdate = this.onupdate; obs.onremove = this.onremove; obs.oninsert = this.oninsert;
		this.tree.observers.push(new WeakRef(obs));
		return obs;
	}

	find_closest_start_iter(tree){
		if(this.asc)
			return tree._tree.lowerBound(tree.key_dummy(this.start_id));
		else {
			const iter = tree._tree.lowerBound(tree.key_dummy(this.start_id));
			if(!iter.data())
				iter.prev();
			while(iter.data()){
				if(tree.key(iter.data()) <= this.start_id)
					return iter;
				iter.prev();
			}
		}
	}

	set(tree, start_id, count){
		//console.log("SET OBS", this, JSON.parse(JSON.stringify(tree)), $state.snapshot(this.data), "\n\n", start_id, count);
		let iter = this.find_closest_start_iter(tree);
		if(!iter){
			this.set_loaded(); // for zero-length data arrays
			return;
		}

		let i = this.inv_items ? this.count - 1 : 0;
		let missing_id = false;
		while(iter.data() && i < this.count && i >= 0){
			const dat = iter.data();
			this.data[this.inv_items ? i-- : i++] = dat;
			if(this.asc){
				iter.next();
				if(iter.data() && tree.key(iter.data()) !== dat.next_id)
					break;
			} else {
				iter.prev();
				if(iter.data() && tree.key(iter.data()) !== dat.prev_id)
					break;
			}
		}
		if(this.inv_items && i >= 0)
			this.data.splice(0, i + 1);

		this.set_loaded();
		//console.log("AFTER SET OBS", this, $state.snapshot(this.data), "\n", tree);
	}

	find(id){
		return Util.bin_search(this.data, (x) => this.tree.key(x) - id, this.asc_items ? false : true);
	}
	remove(id){
		const i = this.find(id);
		if(i !== -1)
			this.data.splice(i, 1);
		console.log("i", i);
		// Try to load more
		if(!this.loading){
			this.loaded = false;
			this.loading = true;
			this.load_func(this.tree, this.start_id, this.count, this.asc);
		}
	}

	get min_id(){
		return Math.min(this.tree.key(this.data[0]), this.tree.key(this.data[this.data.length - 1]));
	}
	get max_id(){
		return Math.max(this.tree.key(this.data[0]), this.tree.key(this.data[this.data.length - 1]));
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

	key; key_dummy;
	constructor(key, key_dummy){
		this.key = key;
		this.key_dummy = key_dummy;
		this._tree = new RBTree((a, b) => {
			return this.key(a) - this.key(b);
		});
	}

	// For loading
	has_enough(obs){
		if(obs.data.length >= obs.count)
			return true;

		if(obs.data.length === 0){
			if(obs.asc ? obs.start_id === 0 : obs.start_id === RangeCache.max_id)
				return false;

			// If start_id is outside of valid ID range, then we should try to load more
			if(obs.asc ? this.min_id !== -1 && obs.start_id < this.min_id
				: this.max_id !== RangeCache.max_id && obs.end_id > this.max_id)
				return false;
		}

		if(obs.asc){
			let end = this._tree.lowerBound(this.key_dummy(obs.max_id));
			if(typeof end.data().next_id === "undefined" && (this.max_id === -1 || this.key(end.data()) < this.max_id))
				return false;
			if(typeof end.data().next_id !== "undefined" && !this._tree.find({id: end.data().next_id}))
				return false;
		} else {
			let beg = this._tree.lowerBound(this.key_dummy(obs.min_id));
			if(typeof beg.data().prev_id === "undefined" && (this.min_id === RangeCache.max_id || this.key(beg.data()) > this.min_id))
				return false;
			if(typeof beg.data().prev_id !== "undefined" && !this._tree.find({id: beg.data().prev_id}))
				return false;
		}
		return true;
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
					f.prev_id = this.key(data[i - 1]);
				if(i < data.length - 1)
					f.next_id = this.key(data[i + 1]);
			} else {
				if(i > 0)
					f.next_id = this.key(data[i - 1]);
				if(i < data.length - 1)
					f.prev_id = this.key(data[i + 1]);
			}
		}
		if(data.length > 0 && count > data.length){
			if(asc){
				if(this.key(data[data.length - 1]) > this.max_id)
					this.max_id = this.key(data[data.length - 1]);
			} else {
				if(this.key(data[data.length - 1]) < this.min_id)
					this.min_id = this.key(data[data.length - 1]);
			}
		}
		const end_id = data.length > 0 ? this.key(data[data.length - 1]) : start_id;
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
						(next) => {next.prev_id = this.key(data); data.next_id = this.key(next);},
						(prev) => {prev.next_id = this.key(data); data.prev_id = this.key(prev);});
			for(const obs of this.get_observers(this.key(data), this.key(data), true)){
				if(obs.oninsert)
					obs.oninsert(obs, this.key(data));
				obs.set(this, this.key(data), obs.count);
			}
		}
	}
	remove(id, update){
		let data = this._tree.find(this.key_dummy(id));

		if(update){
			if(data)
				this.update_neighbours(data,
							(next) => next.prev_id = data.prev_id,
							(prev) => prev.next_id = data.next_id);
			for(const obs of this.get_observers(id, 1)){
				if(obs.onremove)
					obs.onremove(obs, id);
				obs.remove(id);
			}
		}

		this._tree.remove(this.key_dummy(id));
	}
	remove_that(filter){
		let todelete = [];

		let iter = this._tree.iterator();
		iter.next();
		while(iter.data()){
			let data = iter.data();
			if(filter(data))
				todelete.push(data);
			iter.next();
		}

		for(const data of todelete)
			this.remove(this.key(data), true);
	}

	update(data){
		let d = this._tree.find(data);
		if(d){
			for(const key of Object.keys(data))
				d[key] = data[key];

			for(const obs of this.get_observers(this.key(data), this.key(data))){
				if(obs.onupdate)
					obs.onupdate(obs, this.key(data));
				obs.set(this, this.key(data), 1);
			}
		}
	}

	reload(){
		for(const obs of this.get_observers()){
			if(!obs.loading){
				obs.loading = true;
				obs.load_func(this, obs.start_id, obs.count, obs.asc);
			}
		}
	}

	// This function runs in O(NM) + O(KlogN), where N - tree size, M - observer count, K - deleted entries count
	// Seems kinda bad, but realistically N, M and K are rather low, cause only so much entries can be loaded via HTTP in-between trims
	// TODO trim when tree size gets too big
	// TODO delete observers from ranges when iter got past them
	trim(){
		this.min_id = RangeCache.max_id;
		this.max_id = -1;

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
				if(obs.has(this.key(data))){
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

	key;
	constructor(key, key_dummy){
		super();
		if(!key)
			key = (x) => x.id;
		if(!key_dummy)
			key_dummy = (id) => {return {id}};
		this.key = key;
		this.key_dummy = key_dummy;
	}

	_default_state_constructor(){
		return new DataTree(this.key, this.key_dummy);
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
	remove_that(_id, filter){
		const tree = this.get_tree(_id);
		if(tree)
			tree.remove_that(filter);
	}
	update(_id, data){
		const tree = this.get_tree(_id);
		if(tree)
			tree.update(data);
	}

	find(_id, f){
		const tree = this.get_tree(_id);
		if(tree){
			const iter = tree._tree.iterator();
			iter.next();
			while(iter.data()){
				if(f(iter.data()))
					return iter.data();
				iter.next();
			}
		}
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

		if(!nobs.loaded){
			nobs.loading = true;
			load_func(tree, start_id, count, asc);
		}

		return nobs;
	}
	has_state(_id){
		return typeof this.cache[this.state_refs_id(_id)] !== "undefined";
	}
}
