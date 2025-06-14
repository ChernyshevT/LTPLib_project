#!/usr/bin/env python3
#@LISTING:start:probe2d
import sys, os, timeit, shutil, logging, signal
import numpy   as np
import _ltplib as ltp

from datetime     import datetime
from time         import time
from importlib    import import_module
from itertools    import repeat, count
from util.frames  import *
from util.loggers import *
from util.args    import *
from util.plots   import *

from numba import jit

from calcs import rde, wpe, v, me, mp

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

def gen_distro(grid, npunit, **conf):
	nd  = len(grid.step)
	npp = npunit*np.prod(grid.units)
	
	pdata = np.empty([npp, nd+3], dtype=np.float32)
	
	for j in range(0, nd):
		ns,ds = grid.step[j],grid.units[j]
		pdata[:,j] = np.random.uniform(0, ns*ds, size=npp)
	
	for j in range(nd, nd+3):
		# stream velocity
		v0 = conf.get(f"u{chr(ord('x')+j)}", 0)
		# thermal speed
		vt = conf.get(f"vt", 0)
		pdata[:,j] = np.random.normal(v0, vt, size=npp)
	
	return pdata

################################################################################
def main(args, logger):
	ltp.load_backend("default");
	
	# Ke=250 eV, Ti=Te=0.25 eV (H+)
	
	ECHARGE  = 4.803204e-10
	M_4PI_E  = 6.035884e-09
	VEPSILON = 1e-5
	
	ME = 9.109383e-28
	MP = 1.6605402e-24
	E0 = 250
	T0 = 0.25
	
	V0 = np.sqrt(E0 *ECHARGE/150./ME)
	VE = np.sqrt(T0 *ECHARGE/150./ME)
	VI = np.sqrt(T0 *ECHARGE/150./MP)
	print(f"{V0=:e}")

	# the problem's base geometry:
	# ~ nx,mx = 96,  8
	# ~ ny,my = 192, 8
	nx,mx = 192, 16
	ny,my = 192, 16
	dx,dy = 0.0125, 0.0125
	
	
	n_base = 1e12 #cm^-3
	# ~ print(wpe(n0))
	stats = {
		"v_max*dt/dx": max(V0, VE)*args.dt/dx,
		"dt*wce": args.dt*wpe(n_base),
		"dx/rde": dx*wpe(n_base)/max(V0, VE),
		"vte/v0" : VE/V0,
	}
	for k,v in stats.items():
		msg = f"{k} = {v:06.3f}"
		if v<0.5:
			logger.info(msg)
		else:
			logger.warning(msg)
	
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
	 {"KEY":"e-", "CHARGE/MASS": -5.272810e+17},
	 {"KEY":"H+", "CHARGE/MASS": +2.892555e+14},
	]
	, npmax = int(mx*my*args.npunit*(2+args.extra))
	, nargs = 1 + (grid.ndim+3)*2) # extra memory for implicit solver
	# weight coefficient
	wcfft = n_base/args.npunit

	##############################################################################
	# declare electric field vcache & gobal array
	emfield = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = 2) #ExEy
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
		if args.wions:
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
	# now, we will inject the result of previous calculation
	if fpath := args.load:
		f = load_frame(fpath)
		for key,a,b in zip(f.pdescr, f.pindex, f.pindex[1:]):
			pstore.inject({key: f.pdata[a:b]})
		logger.info(f"loaded \"{fpath}\" ({len(pstore)} samples injected)")

	else: # or let's generate samples to inject
		nppin = nx*ny*args.npunit 
		pdata = np.empty([nppin, 2+3], dtype=np.float32)
		# positions
		pdata[:,0] = np.random.uniform(0, nx*dx, size=nppin)
		pdata[:,1] = np.random.uniform(0, ny*dy, size=nppin)
		
		# moving electrons
		pdata[:,2:] = np.random.normal(0, VE, size=[nppin,3])
		pdata[:nppin//2,2] += V0
		pdata[nppin//2:,2] -= V0
		pstore.inject({"e-": pdata})
		
		if args.wions: # cold ion background
			pdata[:,2:] = np.random.normal(0, VI, size=[nppin,3])
			pstore.inject({"H+": pdata})
		logger.info(f"{len(pstore)} samples created")
		
		if fpath := args.save:
			fname = f"{fpath}/init-pdata.zip"
			pdata, pindex, = pstore.extract()
			save_frame(fname, "w"
			, **{"pdata":pdata, "pindex":pindex, "pdescr":pstore.ptlist})
			
	
	##############################################################################
	# run initial step
	run_ppost_step()
	run_field_step()
	
	print(f"{np.mean(g_ptfluid[...,0]):e}")
	
	# arrays to collect frame-data
	f_ptfluid = np.zeros_like(g_ptfluid)
	f_vmap = np.zeros_like(eq.vmap)
	verrs = np.zeros([args.nsub, args.nrep+1], dtype=np.float32)
	
	##############################################################################
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)
	
	# now, run main cycle
	logger.info("start calculation")
	for irun in range(1, args.nrun+1):
		#iclc, tclc = 0, time.time()
		# start new frame [t --> t + args.dt*args.nsub] & clear data
		tframe = args.dt*args.nsub*1e9
		
		# refresh arrays to save
		f_ptfluid[...] = g_ptfluid
		f_vmap[...] = eq.vmap
		verrs[...] = np.nan
		
		logger.info(f"start frame#{irun:06d} ({tframe*(irun-1):07.3f} -> {tframe*irun:07.3f}ns)..")
		for isub in range(1, args.nsub+1):
			
			# run sub-cycle for implicit solver
			for irep in range(0, args.nrep+1):
				# push parts
				run_ppush_step(args.dt, irep>0)
				# obtain density & flows
				run_ppost_step()
				# recalculate field
				verr = run_field_step(); verrs[isub-1, irep] = verr
				
				logger.debug\
				(f"{' 'if irep else '*'}{irun:06d}/{isub:04d}/{irep:02d} verr={verr*300:6.3e} V")
				if irep and verr < VEPSILON:
					break
			
			#end implicit run, collect avg. data
			f_ptfluid[...] += g_ptfluid
			f_vmap[...]    += eq.vmap
		
		logger.info(f"end frame ({len(pstore):} samples)")
		
		# write frame
		if fpath := args.save:
		
			fname = f"{fpath}/frame{irun:06d}.zip"
			frame = {
				"order" : args.order,
				"tstep" : args.dt,
				"step"  : [dx, dy],
				"units" : [nx, ny],
				"tindex": [(irun-1)*args.nsub, irun*args.nsub],
				"vmap"  : np.pad(f_vmap, [(args.order,0), (args.order,0)], mode="wrap")/(args.nsub+1),
				"ne"    : f_ptfluid[..., 0]/(args.nsub+1),
			}
			if args.wions:
				frame = frame | {"ni" : f_ptfluid[..., 1]/(args.nsub+1)}
				
			if args.nrep > 0:
				frame = frame | {"verrs" : verrs}
				
			if args.dump and args.dump[1] == irun:
				args.dump[1] = next(args.dump[0], None)
				
				pdata, pindex, = pstore.extract()
				frame = \
				frame | {"pdata":pdata, "pindex":pindex, "pdescr":pstore.ptlist}
			
			save_frame(fname, "a", **frame)

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
		"type"     : lambda arg: [it:=map(int, arg.split(",")), next(it, None)],
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
		"help"     : "order of form-factor (1=LINEAR/2=QUAD/3=CUBE)"
	},
	"--npunit"   : {
		"help"     : "number of samples per cell",
		"type"     : int,
		"default"  : 250, #1
	},
	"--wions"    : {
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
	"--nrun": {
		"type": int,
		"default": 100, # 1
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
		"help": "0: explicit, >=1: implicit (number of corrector runs)"
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
#@LISTING:end:probe2d
