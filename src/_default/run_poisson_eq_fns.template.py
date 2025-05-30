text = \
r"""extern "C" LIB_EXPORT
f32 SOR_iter{{nd}}_fn (poisson_eq_t<{{nd}}> &eq, f32 w) {
	return run_SOR_iter<{{nd}}>(eq, w);
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
