text = \
"""extern "C" LIB_EXPORT
RET_ERRC ppush{{nd}}{%if CLCRD %}c{% endif %}_{{MODE}}_{{ORDER}}_fn
(const grid_t<{{nd}}> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<{{nd}}, PUSH_MODE::{{MODE}}, FORM_ORDER::{{ORDER}}, {{CLCRD}}>
	(grid, pstore, field, dt, fcode);
}
"""

args = {
	"nd"    : [1,2,3],
	"CLCRD" : [0,1],
	"MODE"  : ["LEAPF", "IMPL0", "IMPLR"],
	"EMFLD" : [1],
	"ORDER" : ["LINE", "QUAD", "CUBE"],
}

################################################################################

def generate(args):
	import itertools

	keys, args = [*zip(*args.items())]
	
	for vals in itertools.product(*args):
		entry = {key:val for key,val in zip(keys, vals)}
		if entry["CLCRD"] and (entry["nd"] != 2 or entry["MODE"] != "LEAPF"):
			continue
		
		yield entry
