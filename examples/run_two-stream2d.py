#!/usr/bin/env python3
#@LISTING:start:two-stream2d
import sys, os, timeit, shutil, logging, signal
import numpy   as np

from datetime     import datetime
from time         import time
from importlib    import import_module
from itertools    import repeat, count
from util.frames  import *
from util.loggers import *
from util.args    import *
#from util.plots   import *

import _ltplib as ltp

class poisson_eq_sp():
	__slots__ = "cmap", "vmap", "cwave", "data",
	
	def __init__ (self, step, units):
		
		self.cmap  = np.zeros(units, dtype=np.float32)
		self.vmap  = np.zeros(units, dtype=np.float32)
		self.cwave = np.zeros(units, dtype=np.float32)
		self.data  = np.zeros(units, dtype=np.complex64)
		
		xs, ys = np.meshgrid(*[np.fft.fftfreq(ns, ds) for ns, ds in zip(units, step)])
		self.cwave.flat[0 ] = 0
		self.cwave.flat[1:] = -0.25/np.pi**2/(xs**2 + ys**2).flat[1:]

	def solve(self):
		self.data[...]  = np.fft.fftn(self.cmap, axes=[0,1])*self.cwave
		self.data[...]  = np.fft.ifftn(self.data, axes=[0,1])
		err = np.max(np.abs(self.vmap-self.data.real))
		self.vmap[...] = self.data.real
		return err

################################################################################
def main(args, logger):

	ME, MP   = 9.109383e-28, 1.6605402e-24 # gram
	ECHARGE  = 4.803204e-10 # statC
	M_4PI_E  = 6.035884e-09
	##############################################################################
	# problem's presets:
	N_BASE   = 1e12 # cm^-3
	E0       = 250  # eV
	T0       = 0.25 # eV
	
	V0  = np.sqrt(E0 * ECHARGE/150./ME)
	VE  = np.sqrt(T0 * ECHARGE/150./ME)
	VI  = np.sqrt(T0 * ECHARGE/150./MP)
	WPE = np.sqrt(M_4PI_E*args.n_base * ECHARGE/ME)

	# the problem's base geometry:
	nx,mx = 192, 16
	ny,my = 192, 16
	dx,dy = 0.0125, 0.0125
	
	stats = {
		"v_max*dt/dx": max(V0, VE)*args.dt/dx,
		"dt*wce": args.dt * WPE,
		"dx/rde": dx*WPE/max(V0, VE),
		"vte/v0" : VE/V0,
	}
	for k,v in stats.items():
		msg = f"{k} = {v:06.3f}"
		if v<0.5:
			logger.info(msg)
		else:
			logger.warning(msg)
	tframe = args.dt*args.nsub*1e9
	logger.info(f"tframe = {tframe:07.3f} ns")
	logger.info(f"order  = {args.order}")
	logger.info(f"npunit = {args.npunit}")
	
	##############################################################################
	ltp.load_backend("default");
	##############################################################################
	# declare grid
	grid = ltp.grid(2
	, step   = [dx,dy]
	, axes   = [[*range(0,nx+1,mx)],[*range(0,ny+1,my)]]
	, nodes  = [(x, y) for x in range(nx//mx) for y in range(ny//my)]
	, loopax = "xy")
	
	##############################################################################
	# declare particle storage
	pstore = ltp.pstore(grid
	, ptinfo = [
	 {"KEY":"e-", "CHARGE/MASS": -ECHARGE/ME},
	 {"KEY":"H+", "CHARGE/MASS": +ECHARGE/MP},
	]
	, npmax = int(mx*my*args.npunit*(2+args.extra))
	, nargs = 1 + (grid.ndim+3)*2) # extra memory for implicit solver
	# weight coefficient
	wcfft = args.n_base/args.npunit

	##############################################################################
	# declare electric field vcache & gobal array
	emfield = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = 2) # Ex Ey
	g_emfield = np.zeros (**emfield.cfg)
	
	##############################################################################
	# declare fluid moments vcache & gobal array
	ptfluid = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = len(pstore.ptlist)) # n
	g_ptfluid = np.zeros (**ptfluid.cfg)

	##############################################################################
	# declare function bindings:
	ppush_fns = [ltp.bind_ppush_fn (pstore, f"ExEy:{mover}", emfield) \
	 for mover in (["IMPL0","IMPLR"] if args.nrep>0 else ["LEAPF"]) 
	]
	ppost_fn = ltp.bind_ppost_fn (pstore, ptfluid, "C")
	
	# remap-fns:
	remap_emfield = ltp.bind_remap_fn (emfield, "<", g_emfield); remap_emfield()
	remap_ptfluid = ltp.bind_remap_fn (ptfluid, ">", g_ptfluid); remap_ptfluid()
	
	##############################################################################
	# declare poisson_eq
	eq = poisson_eq_sp([dx, dy], [nx, ny])
	
	##############################################################################
	# main steps:
	def run_ppush_step(dt, mode):
		ppush_fns[mode](args.dt)()
	
	def run_ppost_step():
		ppost_fn()()
		remap_ptfluid()
		g_ptfluid[...] *= wcfft
	
	def run_field_step():
		ng = args.order
		
		# collect charge density
		eq.cmap[...] = 0
		eq.cmap[...] += g_ptfluid[ng:, ng:, 0]*M_4PI_E
		if args.ions:
			eq.cmap[...] -= g_ptfluid[ng:, ng:, 1]*M_4PI_E
		else:
			eq.cmap[...] -= np.mean(eq.cmap)
		
		#solve
		verr = eq.solve()
		
		# get electric field
		Ex, Ey = np.gradient(-np.pad(eq.vmap, [(ng+1,1), (ng+1,1)], mode="wrap")
		, dx, dy, edge_order=2)
		g_emfield[..., 0] = Ex[1:-1,1:-1]
		g_emfield[..., 1] = Ey[1:-1,1:-1]
		
		# ~ g_emfield[...] = 0 #test
		# put electric field into value cache
		remap_emfield()
		
		return verr
	
		
	##############################################################################
	# now, let's generate samples to inject
	if not (fpath := args.load):
		nppin = nx*ny*args.npunit 
		pdata = np.empty([nppin, 2+3], dtype=np.float32)
		# generate positions
		pdata[:,0] = np.random.uniform(0, nx*dx, size=nppin)
		pdata[:,1] = np.random.uniform(0, ny*dy, size=nppin)
		
		#  generate electron velocities
		pdata[:,2:] = np.random.normal(0, VE, size=[nppin,3])
		pdata[:nppin//2,2] += V0
		pdata[nppin//2:,2] -= V0
		# inject electrons
		pstore.inject({"e-": pdata})
		
		if args.ions: # cold ion background
			# generate ion velocities
			pdata[:,2:] = np.random.normal(0, VI, size=[nppin,3])
			# inject ions
			pstore.inject({"H+": pdata})
		
		logger.info(f"{len(pstore)} samples created")
		
		if fpath := args.save:
			fname = f"{fpath}/pdata-init.zip"
			data, index, = pstore.extract()
			save_frame(fname, "w", **{
			 "data"  : data,
			 "index" : index,
			 "descr" : pstore.ptlist,
			})
	
	else: # inject existing distro
		pdata = load_frame(fpath)
		for key,a,b in zip(f.descr, f.index, f.index[1:]):
			pstore.inject({key: f.data[a:b]})
		logger.info(f"loaded \"{fpath}\" ({len(pstore)} samples injected)")

	##############################################################################
	# run initial step
	run_ppost_step()
	run_field_step()

	##############################################################################
	# define arrays to collect frame-data
	frame_ptfluid = np.zeros_like(g_ptfluid)
	frame_vplasma = np.zeros_like(eq.vmap)
	frame_verrs   = np.zeros([args.nsub, args.nrep+1], dtype=np.float32)
	
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)
	else:
		logger.info("start calculation")
	# now, run main cycle
	for irun in range(1, args.nrun+1):
		#iclc, tclc = 0, time.time()
		# start new frame [t --> t + args.dt*args.nsub] & clear data
		
		
		# refresh arrays to save
		frame_ptfluid[...] = g_ptfluid
		frame_vplasma[...] = eq.vmap
		frame_verrs  [...] = np.nan
		
		logger.info(f"start frame#{irun:06d} ({tframe*(irun-1):07.3f} -> {tframe*irun:07.3f} ns)..")
		for isub in range(1, args.nsub+1):
			
			# run sub-cycle for implicit solver
			for irep in range(0, args.nrep+1):
				# push parts
				run_ppush_step(args.dt, irep>0)
				# obtain density & flows
				run_ppost_step()
				# recalculate field
				verr = run_field_step(); frame_verrs[isub-1, irep] = verr
				
				logger.debug\
				(f"{' 'if irep else ':'}{irun:06d}/{isub:04d}/{irep:02d} verr={verr*300:6.3e} V")
				
				if irep and verr < VEPSILON:
					break
			
			#end implicit run, collect avg. data
			frame_ptfluid[...] += g_ptfluid
			frame_vplasma[...] += eq.vmap
		
		logger.info(f"end frame ({len(pstore):} samples)")
		
		# save frame
		if (fpath := args.save):
			save_frame(f"{fpath}/frame{irun:06d}.zip", "w", **{
			 "args"   : vars(args),
			 "cfg"    : {  
			  "step"   : [dx, dy],
			  "units"  : [nx, ny],
			  "tindex" : [(irun-1)*args.nsub, irun*args.nsub],
			 },
			 "vmap"   : frame_vplasma/(args.nsub+1),
			 "ne"     : frame_ptfluid[..., 0]/(args.nsub+1),
			 **({"ni" : frame_ptfluid[..., 1]/(args.nsub+1)} if args.ions else {}),
			 **({"verrs" : fame_verrs} if args.nrep else {}),
			})
		
		# save samples' dump
		if (fpath := args.save) and irun in args.dump:
			data, index, = pstore.extract()
			save_frame(f"{fpath}/pdata{irun:06d}.zip", "w", **{
			 "data"  : data,
			 "index" : index,
			 "descr" : pstore.ptlist,
			})
			
			

################################################################################
args = {
	"--loglevel" : {
		"type"     : str,
		"required" : False,
		"default"  : "INFO",
		"help"     : "DEBUG/INFO/WARNING/ERROR",
	},
	"--prepare"  : {
		"action"   : argparse.BooleanOptionalAction,
		"help"     : "show configuration & exit",
	},
	"--run"      : {
		"action"   : argparse.BooleanOptionalAction,
		"help"     : "run simulation without asking (task mode)",
	},
	"--save"     : {
		"type"     : str,
		"required" : False,
		"help"     : "path to save the results; default=\"\" (do not save)"
	},
	"--dump"     : {
		"required" : False,
		"default"  : [],
		"type"     : lambda arg: [*map(int, arg.split(","))],
		"help"     : "frame no. to start writing pVDF dumps and"
	},
	"--load"     : {
		"type"     : str,
		"required" : False,
		"help"     : "path to load pVDF dump",
	},
	"--order"    : {
		"type"     : int,
		"default"  : 1,
		"help"     : "form-factor's order (1=LINEAR/2=QUAD/3=CUBE)"
	},
	"--npunit"   : {
		"help"     : "number of samples per cell",
		"type"     : int,
		"default"  : 256, #1
	},
	"--ions"    : {
		"action"   : argparse.BooleanOptionalAction,
		"help"     : "include ions as an active background charge",
	},
	"--extra": {
		"type"     : float,
		"default"  : 2.5,
		"action"   : check_arg(lambda x: x>0, "{} <= 0"),
		"help"     : "extra storage capacity", 
	},
	"--dt": {
		"type"     : float,
		"default"  : 2.5e-12,
		"help"     : "simulation's time-step (t->t+dt)",
	},
	"--n_base"   : {
		"type"     : float,
		"default"  : 1e12,
		"help"     : "base concentration",
	},
	"--nrun": {
		"type": int,
		"default": 500, # 1
		"help": "number of frames to calculate (t->t+dt*nsub)"
	},
	"--nsub": {
		"type": int,
		"default": 10, #2
		"help": "number of sub-steps per frame (t->t+dt)"
	},
	"--nrep": {
		"type": int,
		"default": 0,
		"help": "number of corrector runs (0 for explicit, >=1 for implicit)"
	},
}
################################################################################
def handle_sigterm(n, frame):
	raise SystemExit(signal.strsignal(n))

if __name__ == '__main__':
	from util import setup_logging
	from util.args import *
	
	args = parse_args(sys.argv, args)
	
	setup_logging(level=args.loglevel.upper(), root=True)
	logger= logging.getLogger("probe2d")
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
			
			logger.info(f"will log into \"{fname}\"")
			setup_logging(level=args.loglevel.upper(), root=True\
			, handler = logging.FileHandler(fname, "a", delay=False))
		
		# run program
		sys.exit(main(args, logger))
		
	except (KeyboardInterrupt, SystemExit):
		logger.info("manual exit")
		sys.exit(0)
	
	except Exception as err:
		import colorama
		msg = f"{colorama.Fore.RED+colorama.Style.BRIGHT+colorama.Back.BLACK}{err}{colorama.Style.RESET_ALL}"
		logger.exception(msg)
		sys.exit(1)
#@LISTING:end:two-stream-2d
