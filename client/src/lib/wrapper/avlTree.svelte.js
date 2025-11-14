import {AvlTree} from '@datastructures-js/binary-search-tree';

export default class AvlTreeWrapper extends AvlTree
{
	// The value of this store doesnt matter, what matters is that you can change its value by calling make_dirty(), which will actually force to update things that depend on this state.
	// I tried to find alternative ways, but making _root a state isnt possible cause its a private field; the only alternative is to override ALL methods that change tree structure, and then basically do the same thing, but without make_dirty().
	// Probably should have used JQuery instead of Svelte and all the other state-based crap.
	dirty_flip = $state(false);

	make_dirty()
	{
		this.dirty_flip = !this.dirty_flip;
	}
}
