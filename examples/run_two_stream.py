#!/usr/bin/env python3
#@LISTING:start:two-stream2d
import sys, os, timeit, shutil, logging, signal
import numpy   as np

from functools    import reduce
from datetime     import datetime
from time         import time
#from importlib    import import_module
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
		self.data[...]  = np.fft.fftn  (self.cmap, axes=self.axes) * self.cwave
		self.data[...]  = np.fft.ifftn (self.data, axes=self.axes)
		err = np.max(np.abs(self.vmap-self.data.real))
		self.vmap[...] = self.data.real
		return err

################################################################################
def main(args, logger):


	STATC_V = 2.99792458e2
	ME, AEM = 9.109383e-28, 1.660539e-24 # gram
	ECHARGE = 4.803204e-10 # statC
	M_4PI_E = 6.035884e-09
	CLIGHT  = 2.99792458e+10 # cm/s
	
	##############################################################################
	# problem's presets:
	E0     = args.stream_en  # eV
	T0     = 0.25            # eV
	MLIGHT = ME              # gram
	MHEAVY = 1.00784*AEM     # gram
	
	VE0 = np.sqrt(E0 * 2*ECHARGE/STATC_V/MLIGHT)
	VTE = np.sqrt(T0 * 2*ECHARGE/STATC_V/MLIGHT)
	VTI = np.sqrt(T0 * 2*ECHARGE/STATC_V/MHEAVY)
	WPE = np.sqrt(M_4PI_E*args.n_plasma * ECHARGE/MLIGHT)
	
	##############################################################################
	ltp.load_backend("default");
	
	##############################################################################
	# declare grid
	with open("presets/two_stream_grids.py") as f:
		grid_cfg = eval(f.read())[args.preset]
	grid = ltp.grid(**grid_cfg)

	##############################################################################
	# declare particle storage
	_nsamples = args.npunit*reduce(lambda k, ax: k*(ax[1]-ax[0]), grid.axes, 1)
	_capacity = int(_nsamples*((2 if args.ions else 1) + args.extra))
	
	pstore = ltp.pstore(grid
	, cfg = [
	 {"KEY":"e", "CHARGE/MASS": -ECHARGE/MLIGHT},
	 {"KEY":"i", "CHARGE/MASS": +ECHARGE/MHEAVY},
	]
	, capacity = _capacity
	, vsize = 1 + (grid.nd+3)*2 # extra memory for implicit solver
	)
	
	# weight coefficient
	wcfft = args.n_plasma/args.npunit
	
	##############################################################################
	# declare electric field vcache & gobal array
	emfield = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = grid.nd # Ex Ey (Ez)
	)
	
	##############################################################################
	# declare fluid moments vcache & gobal array
	ptfluid = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = (1+grid.nd)*len(pstore.ptlist) # C Pxx Pyy (Pzz) for each sort
	)

	##############################################################################
	# declare function bindings:
	_descr = "".join(["Ex","Ey","Ez"][:grid.nd])
	ppush_fns = [ltp.bind_ppush_fn (pstore, f"{_descr}:{mover}", emfield) \
	 for mover in ["LEAPF", "IMPL0","IMPLR"]
	]
	
	_descr = "".join(["C","Pxx","Pyy","Pzz"][:1+grid.nd])
	ppost_fn = ltp.bind_ppost_fn (pstore, _descr, ptfluid)
	
	##############################################################################
	# declare poisson_eq
	eq = poisson_eq_sp(grid.units, grid.step)

	def recalc_field():
		slicer1 = [slice(args.order, None, None)]*grid.nd
		slicer2 = [slice(1,          -1,   None)]*grid.nd
		padding = [(args.order+1, 1)]*grid.nd
		
		# collect charge density
		eq.cmap[...] = ptfluid[*slicer1, 0]*M_4PI_E
		if args.ions:
			eq.cmap[...] -= ptfluid[*slicer1, 1+grid.nd]*M_4PI_E
		else:
			eq.cmap[...] -= args.n_plasma*M_4PI_E
		
		# solve
		verr = eq.solve()
		
		# obtain electric field
		_vmap = np.pad(eq.vmap, padding, mode="wrap")
		for i, grad in enumerate(np.gradient(_vmap, *grid.step)):
			emfield[..., i] = -grad[*slicer2]
		
		return verr

	##############################################################################
	
	# ~ stats = {
		# ~ "v_max*dt/dx": max(V0, VE)*args.dt/dx,
		# ~ "dt*wce": args.dt * WPE,
		# ~ "dx/rde": dx*WPE/max(V0, VE),
		# ~ "vte/v0" : VE/V0,
	# ~ }
	# ~ for k,v in stats.items():
		# ~ msg = f"{k} = {v:06.3f}"
		# ~ if v<0.5:
			# ~ logger.info(msg)
		# ~ else:
			# ~ logger.warning(msg)
	# ~ tframe = args.dt*args.nsub*1e9
	# ~ logger.info(f"tframe = {tframe:07.3f} ns")
	# ~ logger.info(f"order  = {args.order}")
	# ~ logger.info(f"npunit = {args.npunit}")

	##############################################################################
	# now, let's generate samples to inject
	if not (fpath := args.load):
		nppin = np.prod(grid.units)*args.npunit
		pdata = np.empty([nppin, grid.nd+3], dtype=np.float32)
		
		# generate positions
		for ax, (ds, ns) in enumerate(zip(grid.step, grid.units)):
			pdata[:, ax] = np.random.uniform(0, ds*ns, size=nppin)
		
		# generate wto groups of electrons
		pdata[:, grid.nd:] = np.random.normal(0, VTE, size=[nppin, 3])
		pdata[:nppin//2, grid.nd] += VE0
		pdata[nppin//2:, grid.nd] -= VE0
		# inject electrons
		pstore.inject("e", pdata)
		
		if args.ions:
			# generate cold ion background
			pdata[:, grid.nd:] = np.random.normal(0, VTI, size=[nppin, 3])
			# inject ions
			pstore.inject("i", pdata)
		
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
		for key,a,b in zip(pdata.descr, pdata.index, pdata.index[1:]):
			pstore.inject(key, pdata.data[a:b])
		nppin = len(pstore)
		logger.info(f"loaded \"{fpath}\" ({len(pstore)} samples injected)")

	##############################################################################
	# arrays to collect data for the each frame
	_ptfluid = np.zeros([args.nsub+1, *ptfluid.shape], dtype=np.float32)
	_vplasma = np.zeros([args.nsub+1, *eq.vmap.shape], dtype=np.float32)
	_emenrgy = np.zeros([args.nsub+1, *emfield.shape[:grid.nd]], dtype=np.float32)
	_errv    = np.zeros([args.nsub, args.nrep+1], dtype=np.float32)
	
	
	_flinfo = []
	for key in pstore.ptlist:
		_flinfo += [f"C_{key}",f"Pxx_{key}", f"Pyy_{key}", f"Pzz_{key}"][:1+grid.nd]

	_cfg = {**vars(args)
	, "grid"   : {"nd": grid.nd, "step": grid.step, "units": grid.units}
	, "nppin"  : np.prod(grid.units)*args.npunit
	, "flinfo" : _flinfo
	}
	
	##############################################################################
	WPE = np.sqrt(M_4PI_E * args.n_plasma * ECHARGE/ME)
	tframe = args.dt*args.nsub*1e9
	
	_units = "x".join(map(str, grid.units))
	_gsize = "x".join([f'{a*b:.3f}' for a,b in zip(grid.step, grid.units)])
	
	if args.dump:
		logger.info(args.dump)
	
	logger.info(f"grid     = {_units}: {_gsize} cm")
	logger.info(f"order    = {args.order}")
	logger.info(f"npunit   = {args.npunit}")
	logger.info(f"nppin    = {nppin}")
	logger.info(f"n_plasma = {args.n_plasma:e} cm^-3")
	logger.info(f"tframe   = {tframe:07.3f} ns")
	logger.info(f"tfull    = {args.nrun*tframe:07.3f} ns")
	logger.info(f"1/δt     = {1/args.dt:e} 1/s")
	# ~ logger.info(f"ωpe      = {WPE:e} 1/s")
	logger.info(f"δt ωpe   = {args.dt*WPE:f}")
	logger.info(f"ve δt/δh = {VE0*args.dt/np.min(grid.shape):f}")
	logger.info(f"VE0      = {VE0:e} cm/s")
	logger.info(f"RDE/δh   = {VE0/WPE/min(*grid.step):f}")
	
	##############################################################################
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)
	
	##############################################################################
	# run initial step
	logger.info("start calculation")
	ppost_fn()
	ptfluid.remap("out")[...] *= wcfft
	recalc_field()
	emfield.remap("in")
	npp = len(pstore)
	
	# now, run main cycle
	dnames = ["ppush", "ppost"]
	dtimes = np.empty([len(dnames)], dtype=np.float64)
	dcalls = np.empty([len(dnames)], dtype=np.uint64)
	for irun in range(args.nstart, args.nrun+1):
		# start new frame [t --> t + args.dt*args.nsub] & clear data
		t0, t1 = (irun-1)*args.nsub*args.dt, irun*args.nsub*args.dt
		logger.info(f"frame#{irun:06d} ({t0*1e9:07.3f} -> {t1*1e9:07.3f} ns)..")
		
		# refresh arrays to save
		_ptfluid[0, ...] = ptfluid[...]
		_vplasma[0, ...] = eq.vmap[...]
		_emenrgy[0, ...] = np.sum(emfield[...]**2, axis=grid.nd)/8/np.pi
		_errv[...] = np.nan
		
		#################
		# run frame-cycle
		dtimes[...], dcalls[...], _tstart = 0, 0, time()
		for isub in range(1, args.nsub+1):
			#####################################################
			# run streaming-phase (sub-cycle for implicit solver)
			for irep in range(0, args.nrep+1):
				mode = (args.nrep>0)+(irep>0)
				# push parts
				t0 = time()
				ppush_fns[mode](args.dt)
				dtimes[0] += time()-t0
				dcalls[0] += npp
				
				# obtain density and pressures
				t0 = time()
				ppost_fn()
				dtimes[1] += time()-t0
				dcalls[1] += npp
				
				ptfluid.remap("out")[...] *= wcfft
				# recalculate field
				verr = recalc_field()
				emfield.remap("in")
				
				logger.debug\
				(f"{' 'if irep else '*'}{irun:06d}/{isub:04d}/{irep:02d}({'E0R'[mode]}) verr={verr:6.3e}")
				
				_errv[isub-1, irep] = verr
				if irep and verr < args.epsilon:
					break
			
			# end sub-cycle, collect data to average
			_ptfluid[isub, ...] = ptfluid[...]
			_vplasma[isub, ...] = eq.vmap[...]
			_emenrgy[isub, ...] = np.sum(emfield[...]**2, axis=grid.nd)/8/np.pi
		
		# end frame cycle
		logger.info(f"end frame, {time()-_tstart:f}s ({len(pstore):} samples)")
		for name, dtpp in zip(dnames, dtimes/dcalls*1e9):
			logger.info(f"{name}: {dtpp:06.3f} ns/part")
		
		# save frame
		if (fpath := args.save):
			save_frame(f"{fpath}/frame{irun:06d}.zip", "w", **{
			 "cfg" : {**_cfg,
			  "tindex": [(irun - 1) * args.nsub, irun * args.nsub],
			 },
			 # plasma potential
			 "vplasma": np.mean(_vplasma, axis=0),
			 # field-energy
			 "emenrgy": np.mean(_emenrgy, axis=0),
			 # concentration, pressures
			 "ptfluid": np.mean(_ptfluid, axis=0),
			 # error-vector
			 **({"errv": _errv} if args.nrep else {}),
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
	"--run"      : {
		"action"   : argparse.BooleanOptionalAction,
		"help"     : "run simulation without asking (task mode)",
	},
	"--save"     : {
		"type"     : lambda fpath: os.path.abspath(os.path.expanduser(fpath)),
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
	"--nstart" : {
		"type"     : int,
		"default"  : 1,
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
	"--stream_en"  : {
		"type"     : float,
		"default"  : 1000,
		"help"     : "initial kinetic energy",
	},
	"--extra": {
		"type"     : float,
		"default"  : 0.75,
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
		"default" : "hires2d",
		"help"    : "select preset: \"hires2d\" or \"lowres3d\""
	},
}
################################################################################
def handle_sigterm(n, frame):
	raise SystemExit(signal.strsignal(n))

if __name__ == '__main__':
	from util import setup_logging
	from util.args import *
	signal.signal(signal.SIGTERM, handle_sigterm)
	
	args = parse_args(sys.argv, args)
	
	setup_logging(level=args.loglevel.upper(), root=True)
	logger= logging.getLogger("problem")
	
	try:

		# check directory
		if args.save and os.path.isdir(args.save) and args.nstart==1:
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
		if fpath := args.save:
			os.makedirs(args.save, exist_ok=True)
			fname = f"{fpath}/runat-{datetime.now().strftime('%Y-%m-%d-%H-%M')}.log"
			
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
