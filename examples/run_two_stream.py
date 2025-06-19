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
	__slots__ = "cmap", "vmap", "cwave", "data", "axes"
	
	def __init__ (self, units, step):
		
		self.axes  = [*range(len(units))]
		self.cmap  = np.zeros(units, dtype=np.float32)
		self.vmap  = np.zeros(units, dtype=np.float32)
		self.cwave = np.zeros(units, dtype=np.float32)
		self.data  = np.zeros(units, dtype=np.complex64)
		
		grids = np.meshgrid(*[np.fft.fftfreq(ns, ds)\
		 for ns, ds in zip(units, step)], indexing='ij')
		sqsum = np.sum([g**2 for g in grids], axis=0)
		
		self.cwave.flat[0 ] = 0
		self.cwave.flat[1:] = -0.25/np.pi**2/sqsum.flat[1:]

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
	E0       = 250  # eV
	T0       = 0.25 # eV
	
	V0  = np.sqrt(E0 * ECHARGE/150./ME)
	VE  = np.sqrt(T0 * ECHARGE/150./ME)
	VI  = np.sqrt(T0 * ECHARGE/150./MP)
	WPE = np.sqrt(M_4PI_E*args.n_plasma * ECHARGE/ME)

	# the problem's base geometry:
	match args.preset:
		case "default2d":
			nx, mx, dx = 192, 16, 0.00625
			ny, my, dy = 192, 16, 0.00625
			grid_conf = {
			 "nd"     : 2,
			 "step"   : [dx,dy],
			 "axes"   : [
			  [*range(0,nx+1,mx)],
			  [*range(0,ny+1,my)],
			 ],
			 "nodes"  : [(x, y)\
			  for x in range(nx//mx)\
			  for y in range(ny//my)\
			 ],
			 "loopax" : "xy",
			}
			node_size = mx*my
		case "lowres3d":
			nx, mx, dx = 48, 8, 0.025
			ny, my, dy = 48, 6, 0.025
			nz, mz, dz = 48, 6, 0.025
			grid_conf = {
			 "nd"     : 3,
			 "step"   : [dx,dy,dz],
			 "axes"   : [
			  [*range(0, nx+1, mx)],
			  [*range(0, ny+1, my)],
			  [*range(0, nz+1, mz)],
			 ],
			 "nodes"  : [(x, y, z)\
			  for x in range(nx//mx)\
			  for y in range(ny//my)\
			  for z in range(nz//mz)\
			 ],
			 "loopax" : "xyz",
			}
			node_size = mx*my*mz
		case _:
			raise ValueError(f"invalid preset \"{args.preset}\"")
		
	logger.info(f"using \"{args.preset}\" preset, {len(grid_conf['nodes'])} nodes")
	
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
	grid = ltp.grid(**grid_conf)
	
	##############################################################################
	# declare particle storage
	pstore = ltp.pstore(grid
	, ptinfo = [
	 {"KEY":"e-", "CHARGE/MASS": -ECHARGE/ME},
	 {"KEY":"H+", "CHARGE/MASS": +ECHARGE/MP},
	]
	, npmax = int(node_size*args.npunit*(2+args.extra))
	, nargs = 1 + (grid.ndim+3)*2) # extra memory for implicit solver
	# weight coefficient
	wcfft = args.n_plasma/args.npunit
	
	##############################################################################
	# declare electric field vcache & gobal array
	emfield = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = len(grid.step)) # Ex Ey (Ez)
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
	remap_emfield = ltp.bind_remap_fn (emfield, "<", g_emfield); #remap_emfield()
	remap_ptfluid = ltp.bind_remap_fn (ptfluid, ">", g_ptfluid); #remap_ptfluid()
	
	##############################################################################
	# declare poisson_eq
	eq = poisson_eq_sp(grid.units, grid.step)
	
	##############################################################################
	# main steps:
	def run_ppush_step(dt, mode):
		ppush_fns[mode](args.dt)()
	
	def run_ppost_step():
		ppost_fn()()
		remap_ptfluid()
		g_ptfluid[...] *= wcfft
	
	def run_field_step():
		slicer1 = [slice(args.order,) for _ in eq.vmap.shape]
		slicer2 = [slice(1, -1)       for _ in eq.vmap.shape]
		padding = [(args.order+1, 1)       for _ in eq.vmap.shape]
		
		# collect charge density
		eq.cmap[...] = 0
		eq.cmap[...] += g_ptfluid[*slicer1, 0]*M_4PI_E
		eq.cmap[...] -= g_ptfluid[*slicer1, 1]*M_4PI_E \
		                if args.ions else args.n_plasma*M_4PI_E
		# solve
		verr = eq.solve()
		
		# obtain electric field
		_vmap = np.pad(eq.vmap, padding, mode="wrap")
		for i, grad in enumerate(np.gradient(_vmap, *grid.step)):
			g_emfield[..., i] = -grad[*slicer2]
		
		# put electric field into value cache
		remap_emfield()
		
		return verr
	
		
	##############################################################################
	# now, let's generate samples to inject
	if not (fpath := args.load):
		nppin = np.prod(grid.units)*args.npunit 
		pdata = np.empty([nppin, grid.ndim+3], dtype=np.float32)
		
		# generate positions
		for ax, (ds, ns) in enumerate(zip(grid.step, grid.units)):
			pdata[:, ax] = np.random.uniform(0, ds*ns, size=nppin)
		
		# generate electron velocities
		pdata[:, grid.ndim:] = np.random.normal(0, VE, size=[nppin,3])
		pdata[:nppin//2, grid.ndim] += V0
		pdata[nppin//2:, grid.ndim] -= V0
		# inject electrons
		pstore.inject({"e-": pdata})
		
		if args.ions: # cold ion background
			# generate ion velocities
			pdata[:, grid.ndim:] = np.random.normal(0, VI, size=[nppin,3])
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
	# define arrays to collect data for each frame-data
	frame_ptfluid = np.zeros([args.nsub+1, *ptfluid.shape], dtype=np.float32)
	frame_vplasma = np.zeros([args.nsub+1, *eq.vmap.shape], dtype=np.float32)
	frame_errv    = np.zeros([args.nsub, args.nrep+1],      dtype=np.float32)
	
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)
	else:
		logger.info("start calculation")
	# now, run main cycle
	ts = np.empty([3], dtype=np.float64)
	for irun in range(1, args.nrun+1):
		# start new frame [t --> t + args.dt*args.nsub] & clear data
		
		t0, t1 = (irun-1)*args.nsub*args.dt, irun*args.nsub*args.dt
		logger.info(f"frame#{irun:06d} ({t0*1e9:07.3f} -> {t1*1e9:07.3f} ns)..")
		
		# refresh arrays to save
		frame_ptfluid[0, ...] = g_ptfluid
		frame_vplasma[0, ...] = eq.vmap
		frame_errv[...] = np.nan
		
		# run frame-cycle
		for isub in range(1, args.nsub+1):
			# run sub-cycle for implicit solver
			for irep in range(0, args.nrep+1):
				# push parts
				t0 = time()
				run_ppush_step(args.dt, irep>0)
				# obtain density & flows
				t1 = time()
				run_ppost_step()
				# recalculate field
				verr = run_field_step()
				frame_errv[isub-1, irep] = verr
				t2 = time()
				
				logger.debug\
				(f"{' 'if irep else '*'}{irun:06d}/{isub:04d}/{irep:02d} verr={verr:6.3e}")
				
				if irep and verr < args.epsilon:
					break
			
			#end implicit run, collect avg. data
			frame_ptfluid[isub, ...] = g_ptfluid
			frame_vplasma[isub, ...] = eq.vmap
		
		# end frame cycle
		logger.info(f"end frame ({len(pstore):} samples)")
		
		# save frame
		if (fpath := args.save):
			save_frame(f"{fpath}/frame{irun:06d}.zip", "w",
			 cfg = dict(**vars(args),
			       step   = grid.step,
			       units  = grid.units,
			       tindex = [(irun - 1) * args.nsub, irun * args.nsub],
			 ),
			 # plasma potential:
			 vplasma = np.mean(frame_vplasma, axis=0)
			           if args.mean else frame_vplasma
			 ,
			 # electron concentration:
			 ne  = np.mean(frame_ptfluid[..., 0], axis=0)
			       if args.mean else frame_ptfluid[..., 0]
			 ,
			 # ion concentration:
			 **(dict(ni = np.mean(frame_ptfluid[..., 1], axis=0)
			              if args.mean else frame_ptfluid[..., 1]
			 ) if args.ions else {}),
			 # error-vector
			 **(dict(errv = frame_errv) if args.nrep else {}),
			)

		
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
		"action"   : "store_false",
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
		"type"     : int,
		"required" : False,
		"default"  : [],
		"nargs"    : "+",
		"help"     : "frame list to writing pVDF dumps",
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
		"action"   : "store_true",
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
		"help"     : "simulation's time-step (t->t+dt, s)",
	},
	"--n_plasma"   : {
		"type"     : float,
		"default"  : 1e12,
		"help"     : "plsama concentration (cm^-3)",
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
	"--nrep"    : {
		"type"    : int,
		"default" : 0,
		"help"    : "number of corrector runs (0 for explicit, >=1 for implicit)"
	},
	"--epsilon" : {
		"type"    : float,
		"default" : 0,
		"help"    : "epsilon to stop implicit solver earlier (V)"
	},
	"--preset"  : {
		"type"    : str,
		"default" : "default2d",
		"help"    : "select preset: \"default2d\" or \"lowres3d\""
	},
	"--no-mean"    : {
		"dest"    : "mean",
		"action"  : "store_false",
	}
}
################################################################################
def handle_sigterm(n, frame):
	raise SystemExit(signal.strsignal(n))

if __name__ == '__main__':
	from util import setup_logging
	from util.args import *
	
	args = parse_args(sys.argv, args)
	
	print(args)
	
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
