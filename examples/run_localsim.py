#!/usr/bin/env python3

import sys, os, timeit, shutil, logging, signal
import numpy   as np
import _ltplib as ltp

from datetime     import datetime
from time         import time
from importlib    import import_module
from util.frames  import *
from util.loggers import *
from util.args    import *

################################################################################
def handle_sigterm(n, frame):
	raise SystemExit(signal.strsignal(n))

################################################################################
def inject_parts (pstore, npp_in, x0, x1, vt):
	distro  = np.empty([npp_in, 4], dtype=np.float32)
	# pxy
	distro[:, 0] = np.random.uniform(x0, x1, size=npp_in)
	# vxyz
	distro[:, 1] = np.random.normal(size=npp_in, scale=vt)
	distro[:, 2] = np.random.normal(size=npp_in, scale=vt)
	distro[:, 3] = np.random.normal(size=npp_in, scale=vt)
	# inject
	pstore.inject({"e":distro})
	# print(pstore.index)

################################################################################

def main(args):
	ltp.load_backend("default");
	
	for k,v in vars(args).items():
		if v is None:
			continue
		if type(v) != str:
			logger.info(f"{k:<8} : {v}")
		else:
			logger.info(f"{k:<8} : \"{v}\"")

	##############################################################################
	# init csection database:
	bginfo = [(lambda x: (x[0], float(x[1])))(x.split(":")) for x in args.bginfo]
	keys, fracs = zip(*bginfo); fracs = np.array(fracs)/sum(fracs)
	cs_cfg = [
	 {"TYPE":"PARTICLE", "KEY":"e", "ENCFFT": 2.842815e-16, #CGS (cm/s)² -> eV
	 },
	]
	cs_kws = {
	 "max_energy" : args.max_energy,
	 "ptdescr"    : "e",
	 "rescale"    : 1e4, #CGS (m² -> cm²)
	}
	for key in keys:
		fname = f"csections_db/{key}.py"
		with open(fname) as f:
			cs_cfg += eval(f.read() \
			, {"fpath":os.path.dirname(fname), **vars(import_module("math"))})
	cset = ltp.csection_set(cs_cfg, **cs_kws)

	##############################################################################
	E0 = args.EN*args.nbg*1e-17 #SI Td -> V/cm
	B0 = args.BN*args.nbg*1e-17 #SI Hx -> Gauss
	
	chinfo = []
	for entry, info in zip(cset, cset.chinfo):
		descr = info
		if entry.comment:
			descr += f" {entry.comment}"
		if entry.enth > 0:
			descr += f" ({entry.enth:.2e} eV)"
		chinfo.append(descr)
	
	cfg = {
			 "units"  : "CGS",
			 "TE"     : args.te,
			 "N0"     : args.nbg,
			 "EN"     : args.EN,
			 "BN"     : args.BN,
			 "E0"     : E0,
			 "B0"     : B0,
			 "fracs"  : fracs, 
			 "tstep"  : args.dt,
			 "tframe" : args.dt*args.nsub,
			 "chinfo" : chinfo,
	}
	for k,v in cfg.items():
		unitx = {"N0":" 1/cm3", "EN":" Td", "BN":" Hx", "E0":" V/cm", "B0":" G"}.get(k,"") #CGS
		logger.info(f"{k:<8} : {v}"+unitx)
	
	#####################
	# create fake 1d grid
	grid = ltp.grid(step=[1]
	, axes=[[*range(args.nodes+1)]]
	, nodes=[[i] for i in range(args.nodes)]
	, flags="LOOPX")

	#######################
	# init particle storage
	pstore_cfg = {
		"npmax" : int(args.npp*(1.0 + args.extra)/args.nodes),
		"ptinfo": [
			{"KEY":"e", "CHARGE/MASS": -5.272810e+17}, #CGS
		],
	}
	pstore = ltp.pstore(grid, **pstore_cfg)
	
	######################################################
	# declare value caches and corresponding numpy-arrays
	emfield = ltp.vcache(grid, "f32", vsize=2, order=1)
	bgrnd   = ltp.vcache(grid, "f32", vsize=len(cset.bglist), order=0)
	cfreq   = ltp.vcache(grid, "u32", vsize=len(cset), order=0)

	g_emfield = np.zeros(**emfield.cfg)
	g_cfreq   = np.zeros(**cfreq.cfg)
	m_cfreq   = np.zeros(**{**cfreq.cfg, **{"dtype":np.float32}})
	
	# set em-field
	g_emfield[..., 0] = -E0/2.99792458e2  # (V/cm -> statV/cm) #CGS
	g_emfield[..., 1] = +B0/2.99792458e10 # (G -> G*s/cm)      #CGS
	ltp.bind_remap_fn(emfield, "<", g_emfield)()
	
	# set background
	g_bgrnd   = np.ones (**bgrnd.cfg)
	for j, frac in enumerate(fracs):
		g_bgrnd[..., j] = args.nbg*frac
	ltp.bind_remap_fn(bgrnd, "<", g_bgrnd)()
	
	##############################################################################
	ppush_fn = ltp.bind_ppush_fn (pstore, "ExBz:LEAPF", emfield)
	mcsim_fn = ltp.bind_mcsim_fn (pstore, cfreq, cset, bgrnd)
	remap_cfreq_out = ltp.bind_remap_fn (cfreq, ">", g_cfreq)
	remap_cfreq_in  = ltp.bind_remap_fn (cfreq, "<", g_cfreq)

	##############################################################################
	if args.run == False or not (args.run or input(f"run? [y|yes] or.. ") in ["y", "yes"]):
		exit(0)
	
	tdescr = ["mcsim_fn","ppush_fn"]
	ts = np.empty([1+len(tdescr)], dtype=np.float64)

	# inject samples
	logger.info("inject samples..")
	inject_parts(pstore, args.npp, 0, args.nodes, np.sqrt(args.te/2.842815e-16)) #CGS
	
	# main cycle
	logger.info("start calculation..")
	for irun in range(1, args.nrun+1):
		
		t0, t1 = (irun-1)*args.nsub*args.dt, irun*args.nsub*args.dt
		logger.info(f"frame#{irun:06d} ({t0*1e9:10.4f} -> {t1*1e9:10.4f}ns)..")
		
		tclc = time()
		# start frame [t --> t + dt*args.nsub], clear data
		g_cfreq[...] = 0; remap_cfreq_in() # set zero to cfreq cache
		
		ts[:] = 0.0; j1 = 0
		for isub in range (1, args.nsub+1):
			
			t0 = time()
			seed = np.random.randint(0xFFFFFFFF, dtype=np.uint32)
			mcsim_fn(args.dt, seed)
			t1 = time()
			ppush_fn(args.dt)
			t2 = time()
			ts[1] += t1-t0
			ts[2] += t2-t1
			npp = len(pstore); j1 += npp
		
		# end frame
		tclc = time()-tclc
		logger.info(f"time {'frame':<10} : {tclc:>6.2f} s, ({npp} samples)")
		for key,tval in zip(tdescr, ts[1:]):
			logger.info(f"time {key:<10} : {tval:>6.2f} s, {tval/j1*1e9:>6.2f} ns/sample")
		
		pts, _ = pstore.extract()
		# keep ensemble's size constant
		if args.resample and args.npp != npp:
			pstore.reset()
			np.random.shuffle(pts)
			pstore.inject({"e": pts[:min(args.npp, npp)]})
			if args.npp > npp:
				pstore.inject({"e": pts[:args.npp-npp]})
			logger.info(f"resample {npp} -> {args.npp}")
		
		remap_cfreq_out();
		outv = {
			"pdata": pts[:, 1:],
			"cfreq": np.sum(g_cfreq, axis=0, dtype=np.float32)/j1/args.dt,
			"fluid": {
				# cm/s
				"u0": np.sqrt(np.mean(np.sum(pts[:, 1:]**2, axis=1))),
				"ux": np.mean(pts[:, 1]),
				"uy": np.mean(pts[:, 2]),
				"uz": np.mean(pts[:, 3]),
			},
			"cfg": {**cfg,
				"tindex": [(irun-1)*args.nsub, irun*args.nsub],
			},
		}
		for k,v in outv["fluid"].items():
			logger.info(f"fluid.{k} = {v:+e} cm/s") #CGS
		logger.info(f"fluid.emean =  {(outv['fluid']['u0']**2) * 2.842815e-16:e} eV")
		
		for info,freq in zip(cset.chinfo, outv['cfreq']):
			logger.info(f"{info:<48}{freq:e}")
		
		# write frame
		if fpath := args.save:
			fname = f"{fpath}/frame{irun:06d}.zip"
			save_frame(fname, "w", **outv)

################################################################################
args = {
	"--bginfo": {
		"help"     : "list with KEY:FRACTION separated by spaces",
		"nargs"    : "+",
		"type"     : str,
		"required" : True,
	},
	"--nbg": {
		"help"     : "sum of backgrounds' densities, (1/cm^3)", #CGS
		"type"     : float,
		"default"  : 1e17,
		"action"   : check_arg(lambda x: x>0, "should be positive"),
	},
	"--EN": {
		"help"     : "reduced electric field (E_x/n_0, Td)",
		"type"     : float,
		"required" : True,
	},
	"--BN": {
		"help"     : "reduced magnetic field (B_z/n_0, Hx)",
		"type"     : float,
		"default"  : 0,
	},
	"--max_energy" : {
		"help"     : "energy limit for cross-section database, (eV)",
		"type"     : float,
		"required" : True,
		"action"   : check_arg(lambda x: x>0, "should be positive"),
	},
	"--te": {
		"help" : "initial temperature of electron ensemble",
		"type": float,
		"default": 0,
		"action"   : check_arg(lambda x: x>0, "should be positive"),
	},
	"--dt": {
		"help"     : "simulation's time-step (t->t+dt)",
		"type"     : float,
		"default"  : 2.5e-12,
	},
	"--nodes": {
		"help"     : "number of cpu-nodes (2 at least)",
		"type"     : int,
		"default"  : 1,
		"action"   : check_arg(lambda x: x>0, "{} <= 0"),
	},
	"--nsub": {
		"help": "number of sub-steps per frame (t -> t+dt*nsub)",
		"type": int,
		"default" : 2,
		"action"   : check_arg(lambda x: x>=2, "{} < 2"),
	},
	"--nrun": {
		"help": "number of frames to calculate (t -> t+dt*nsub*nrun)",
		"type": int,
		"default": 1,
	},
	"--npp": {
		"help": "number of samples",
		"type": int,
		"default": 100000,
	},
	"--extra": {
		"help"     : "extra storage capacity (multiplication factor), some minimal value is required for even if there are no ionization events", 
		"type"     : float,
		"default"  : 0.25,
		"action"   : check_arg(lambda x: x>0, "sould not be zero"),
	},
	"--resample" : {
		"help": "resample eVDF each frame to keep size of ensemble constant",
		"action": argparse.BooleanOptionalAction,
	},
	"--save": {
		"help"     : "path to save the results, default=\"\" (do not save)",
		"type"     : str,
		"default"  : "",
	},
	"--run": {
		"help": "run simulation without asking",
		"action": argparse.BooleanOptionalAction,
	},
	"--loglevel": {
		"help": "logging level: DEBUG|INFO|WARNING|ERROR",
		"type": str,
		"required": False,
		"default": "INFO",
	},
}

################################################################################
if __name__ == '__main__':
	
	args   = parse_args(sys.argv, args)
	logger = get_logger()
	setup_logging(level=args.loglevel.upper(), root=True)
	
	signal.signal(signal.SIGTERM, handle_sigterm)
	try:
		# check directory
		if args.save and os.path.isdir(args.save):
			if not args.run:
				if input\
				(f"dir \"{args.save}\" already exists, delete contents? [y|yes] or.. ")\
				in ["y", "yes"]:
					shutil.rmtree(args.save)
				else:
					exit(0)
			else:
				raise RuntimeError(f"dir \"{args.save}\" already exists!")
		
		# create log-file
		if args.save:
			os.makedirs(args.save, exist_ok=True)
			fname\
			= f"{args.save}/runat-{datetime.now().strftime('%Y-%m-%d-%H-%M')}.log"
			
			logger.info(f"will write to \"{fname}\"")
			setup_logging(level=args.loglevel.upper(), root=True\
			, handler = logging.FileHandler(fname, "a", delay=False))
		
		# run program
		sys.exit(main(args))
		
	except (KeyboardInterrupt, SystemExit):
		logger.info("manual exit")
		sys.exit(0)
	
	except Exception as err:
		import colorama
		msg = f"{colorama.Fore.RED+colorama.Style.BRIGHT+colorama.Back.BLACK}{err}{colorama.Style.RESET_ALL}"
		logger.exception(msg)
		sys.exit(1)

