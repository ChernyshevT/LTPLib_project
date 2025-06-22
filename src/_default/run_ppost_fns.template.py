text = \
r"""extern "C" LIB_EXPORT
u32 ppost{{nd}}_{{order}}_fn
(const grid_t<{{nd}}> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<{{nd}}, FORM_ORDER::{{order}}>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
"""

args = {
	"nd"    : [1,2,3],
	"order" : ["LINE", "QUAD", "CUBE"]
}

################################################################################

def generate(args):
	import itertools

	keys, args = [*zip(*args.items())]
	
	for vals in itertools.product(*args):
		yield {key:val for key,val in zip(keys, vals)}
