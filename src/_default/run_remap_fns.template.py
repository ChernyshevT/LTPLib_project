text = \
"""extern "C" LIB_EXPORT
void remap{{nd}}{{'f32' if type=='f32' else 'u32'}}_{{mode}}_fn
(const grid_t<{{nd}}> &grid, vcache_t<{{type}}> &latt, {{type}} *g_data ) {
	run_remap_to_{{mode|lower}}<{{nd}}, {{type}}>
	(grid, latt, g_data);
}
"""

args = {
	"nd"    : [1,2,3],
	"mode"  : ["NODES", "ARRAY"],
	"type"  : ["f32", "u32"],
}

################################################################################

def generate(args):
	import itertools

	keys, args = [*zip(*args.items())]
	
	for vals in itertools.product(*args):
		yield {key:val for key,val in zip(keys, vals)}
