text = \
"""extern "C" LIB_EXPORT
u32 mcsim{{nd}}_fn (
const grid_t<{{nd}}> &grid, pstore_t &pstore, vcache_t<u32> &events, const csection_set_t &cset, const vcache_t<f32> &bgrnd, f32 dt, u32 seed) {
	u32 flags;
	flags = run_mcsim<{{nd}}> (grid, pstore, events, cset, bgrnd, dt, seed);
	if (flags) {
		return ERR_CODE::MCSIM_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
"""

args = {
	"nd" : [1,2,3],
}

################################################################################

def generate(args):
	import itertools

	keys, args = [*zip(*args.items())]
	
	for vals in itertools.product(*args):
		yield {key:val for key,val in zip(keys, vals)}
