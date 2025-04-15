#!/usr/bin/env python3

import jinja2, os

################################################################################

def gen_fns (text, args, generate, **_):
	
	render = lambda entry: jinja2.Template(text).render(**entry)
	return "\n".join([f"/* #{j:02d} "+"*"*71+"*/\n"+render(entry)\
	       for j,entry in enumerate(generate(args), 1)])

################################################################################

typename = {
	"f32":    "f32",
	"u32": "u32",
}

def main(args):
	
	for arg in args[1:]:
		srcname = arg
		dstname = arg.replace(".template.py",".cxx")
		
		with open(srcname, "r") as fsrc:
			params = {}; exec(fsrc.read(), {}, params)
			c_code = gen_fns(**params)

		msg = f"\"{srcname}\" -> \"{dstname}\""
		if os.path.exists(dstname):
			msg = msg+" (overwrite)"
		print(msg)

		with open(dstname, "w") as fdst:
			fdst.write(c_code)

	return 0

################################################################################

if __name__ == '__main__':
	import sys
	sys.exit(main(sys.argv))
