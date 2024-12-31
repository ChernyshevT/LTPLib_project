text = \
"""extern "C" LIB_EXPORT
RET_ERRC ppost{{nd}}_{{mode}}_{{order}}_fn
(const grid_t<{{nd}}> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<{{nd}}, POST_MODE::{{mode}}, FORM_ORDER::{{order}}>
	(grid, pstore, ptfluid);
}
"""

args = {
	"nd"    : [1,2,3],
	"mode"  : ["C", "CF", "CFP", "CFPS"],
	"order" : ["LINE", "QUAD", "CUBE"]
}

################################################################################

def generate(args):
	import itertools

	keys, args = [*zip(*args.items())]
	
	for vals in itertools.product(*args):
		yield {key:val for key,val in zip(keys, vals)}
