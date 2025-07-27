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
		self.data[...]  = np.fft.fftn  (self.cmap, axes=self.axes) * self.cwave
		self.data[...]  = np.fft.ifftn (self.data, axes=self.axes)
		err = np.max(np.abs(self.vmap-self.data.real))
		self.vmap[...] = self.data.real
		return err

################################################################################
def main(args, logger):

	ME, MP   = 9.109383e-28, 1.6605402e-24 # gram
	ECHARGE  = 4.803204e-10 # statC
	M_4PI_E  = 6.035884e-09
	CLIGHT   = 2.99792458e+10

	##############################################################################
	ltp.load_backend("default")
	
	##############################################################################
	# load gas presets
	bginfo = [(lambda x: (x[0], float(x[1])))(x.split(":")) for x in args.bginfo]
	keys, fracs = zip(*bginfo)
	cs_cfg = [
	 {"TYPE":"PARTICLE", "KEY":"e",
	  "ENCFFT": 2.842815e-16}, #CGS (cm/s)² -> eV
	]
	for key in keys:
		fname = f"csections_db/{key}.py"
		lvars = {"fpath": os.path.dirname(fname), **vars(import_module("math"))}
		with open(fname, "r") as f:
			cs_cfg += eval(f.read(), lvars)
	
	##############################################################################
	# init csection database
	cset = ltp.csection_set(cs_cfg
	, max_energy = args.max_energy
	, ptdescr = "e"
	, rescale = 1e4 #CGS (m² -> cm²)
	)

	# ############################################################################
	# the problem's base geometry:
	nx, mx, dx = 192, 16, 1/(2<<9)
	ny, my, dy = 192, 16, 1/(2<<9)
	node_size = mx*my
	# declare grid
	grid = ltp.grid(2
	, step = [dx,dy]
	, axes = [[*range(0,nx+1,mx)], [*range(0,ny+1,my)]]
	, nodes = [(x,y) for x in range(nx//mx) for y in range(ny//my)]
	, loopax = "xy")
	
	##############################################################################
	# declare particle storage
	nppin = np.prod(grid.units)*args.npunit
	npmax = int(node_size*args.npunit*(2+args.extra))
	
	pstore = ltp.pstore(grid
	, ptinfo = [
	 {"KEY":"e", "CHARGE/MASS": -ECHARGE/ME},
	]
	, npmax = npmax
	, nargs = 1 + (grid.ndim+3)*2) # extra memory for implicit solver

	##############################################################################
	# declare electric field vcache
	emfield = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = 1+len(grid.step)) # Bz Ex Ey (Ez)

	##############################################################################
	# declare fluid moments vcache
	ptfluid = ltp.vcache (grid
	, dtype = "f32"
	, order = args.order
	, vsize = len(pstore.ptlist)*4) # C Fx Fy KEn

	##############################################################################
	# declare vcaches for event-counter and background concentration
	events = ltp.vcache(grid, dtype="u32", vsize=len(cset))
	bgrnd  = ltp.vcache(grid, dtype="f32", vsize=len(cset.bglist))
	
	##############################################################################
	# set-up background
	#xs, ys = np.mgrid[0:nx, 0:ny]
	for j, frac in enumerate(fracs):
		#bgrnd[..., j] = args.n_bgrnd*frac*(np.sin(xs*2*np.pi/nx)*np.sin(ys*2*np.pi/ny))**2
		bgrnd[..., j] = args.n_bgrnd*frac
	bgrnd.remap("in")
	
	##############################################################################
	# declare function bindings:
	mcsim_fn = ltp.bind_mcsim_fn(pstore, events, cset, bgrnd)
	
	ppost_fn = ltp.bind_ppost_fn(pstore, "C KEn Fx Fy", ptfluid)
	
	emf_descr = "BzExEyEz"[:(1+grid.ndim)*2]
	ppush_fns = [ltp.bind_ppush_fn(pstore, f"{emf_descr}:{scheme}", emfield) \
	 for scheme in ["LEAPF", "IMPL0","IMPLR"]
	]

	##############################################################################
	# get actual fields
	E0 = args.EN*args.n_bgrnd*1e-17 #SI Td -> V/cm
	B0 = args.BN*args.n_bgrnd*1e-17 #SI Hx -> Gauss

	##############################################################################
	# declare poisson_eq
	eq = poisson_eq_sp(grid.units, grid.step)
	
	def recalc_field():
		slicer1 = [slice(args.order, None, None)  for _ in eq.vmap.shape]
		slicer2 = [slice(1,          -1,   None)  for _ in eq.vmap.shape]
		padding = [(args.order+1, 1)  for _ in eq.vmap.shape]
		
		# collect charge density
		eq.cmap[...] = (ptfluid[*slicer1, 0] - args.n_plasma)*M_4PI_E

		# solve
		verr = eq.solve()
		
		# obtain self-sustained electric field
		_vmap = np.pad(eq.vmap, padding, mode="wrap")
		for i, grad in enumerate(np.gradient(_vmap, *grid.step), 1):
			emfield[..., i] = -grad[*slicer2]
		# apply external fields
		emfield[..., 0] += B0/CLIGHT # Gauss -> Gauss*s/cm
		emfield[..., 1] -= E0/2.99792458e2  # V/cm -> statV/cm
		
		return verr
	
		
	##############################################################################
	# now, let's generate samples to inject
	if not (fpath := args.load):
		pdata = np.zeros([nppin, grid.ndim+3], dtype=np.float32)
		
		# generate electrons' positions
		for ax, (ds, ns) in enumerate(zip(grid.step, grid.units)):
			pdata[:, ax] = np.random.uniform(0, ds*ns, size=nppin)
		
		# inject electrons
		pstore.inject({"e": pdata})
		logger.info(f"{len(pstore)} samples created")
		
		if fpath := args.save:
			fname = f"{fpath}/pdata-init.zip"
			data, index = pstore.extract()
			save_frame(fname, "w", **{
			 "data"  : data,
			 "index" : index,
			 "descr" : pstore.ptlist,
			})
	
	else: # inject existing distro
		pdata = load_frame(fpath)
		for key,a,b in zip(pdata.descr, pdata.index, pdata.index[1:]):
			pstore.inject({key: pdata.data[a:b]})
		logger.info(f"loaded \"{fpath}\" ({len(pstore)} samples injected)")

	##############################################################################
	# arrays to collect data for the each frame
	_ptfluid = np.zeros([args.nsub+1, *ptfluid.shape], dtype=np.float32)
	_vplasma = np.zeros([args.nsub+1, *eq.vmap.shape], dtype=np.float32)
	_emenrgy = np.zeros([args.nsub+1, *emfield.shape[:2]], dtype=np.float32)
	_errv    = np.zeros([args.nsub, args.nrep+1], dtype=np.float32)
	# collision event frequencies
	_evtfreq = np.zeros(events.shape, dtype=np.float32)

	# channels' info to write into frame
	chinfo = []
	for entry, info in zip(cset, cset.chinfo):
		descr = info
		if entry.comment:
			descr += f" {entry.comment}"
		if entry.enth > 0:
			descr += f" ({entry.enth:.2e} eV)"
		chinfo.append(descr)
	
	cfg = {**vars(args),
	 "step"     : grid.step,
	 "units"    : grid.units,
	 "nppin"    : nppin,
	 "E0"       : E0,
	 "B0"       : B0,
	 "chinfo"   : chinfo,
	 "flinfo"   : ["C","KEn", "Fx","Fy"],
	}
	
	WCE = ECHARGE*B0/ME/CLIGHT
	WPE = np.sqrt(M_4PI_E * args.n_plasma * ECHARGE/ME)
	WMX = args.n_bgrnd * cset[len(cset)-1].rate_max
	RCE = 2*(E0*ME/ECHARGE)*(CLIGHT/B0)**2 if B0 else np.nan
	tframe = args.dt*args.nsub*1e9
	
	logger.info(f"use default2d preset: {nx}x{ny} ({dx*nx}x{dy*ny} cm)")
	logger.info(f"order    = {args.order}")
	logger.info(f"npunit   = {args.npunit}")
	logger.info(f"nppin    = {nppin}")
	logger.info(f"n_plasma = {args.n_plasma:e} cm^-3")
	logger.info(f"n_bgrnd  = {args.n_bgrnd:e} cm^-3")
	logger.info(f"tframe   = {tframe:07.3f} ns")
	logger.info(f"1/δt     = {1/args.dt:e} 1/s")
	logger.info(f"ωpe      = {WPE:e} 1/s")
	if B0 != 0:
		logger.info(f"ωce      = {WCE:e} 1/s")
	logger.info(f"n0∑ϑ     = {WMX:e} 1/s")
	logger.info(f"E0       = {E0:e} V/cm")
	if B0 != 0:
		logger.info(f"B0       = {B0:e} G")
		logger.info(f"rce      = {RCE:f} cm") 
		
	
	##############################################################################
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)
	##############################################################################
	# run initial step
	logger.info("start calculation")
	ppost_fn()
	ptfluid.remap("out")[...] *= args.n_plasma/args.npunit*nppin/len(pstore)
	recalc_field()
	emfield.remap("in")
	
	##############################################################################
	# now, run the main cycle
	ts = np.empty([3], dtype=np.float64)
	for irun in range(args.nstart, args.nrun+1):
		# start new frame [t --> t + args.dt*args.nsub] & clear data
		t0, t1 = (irun-1)*args.nsub*args.dt, irun*args.nsub*args.dt
		logger.info(f"frame#{irun:06d} ({t0*1e9:07.3f} -> {t1*1e9:07.3f} ns)..")
		
		# refresh arrays to save
		_ptfluid[0, ...] = ptfluid[...]
		_vplasma[0, ...] = eq.vmap[...]
		_emenrgy[0, ...] = np.sum(emfield[..., 1:]**2, axis=2)
		_errv[...] = np.nan
		
		# reset collision counter
		events.reset()
		np_counter = 0
		
		#################
		# run frame-cycle
		for isub in range(1, args.nsub+1):

			#####################
			# run collision-phase
			seed = np.random.randint(0xFFFFFFFF, dtype=np.uint32)
			mcsim_fn(args.dt, seed)
			npp = len(pstore); np_counter += npp
			# [!] we need this hack because electrons spawn without paired ions
			ptfluid.remap("out")[...] *= args.n_plasma/args.npunit*nppin/npp
			recalc_field()
			emfield.remap("in")

			#####################################################
			# run streaming-phase (sub-cycle for implicit solver)
			for irep in range(0, args.nrep+1):
				# push particles
				mode = (args.nrep>0)+(irep>0)
				ppush_fns[mode](args.dt)
				npp = len(pstore)
				# obtain density & flows & kinetic energy
				ppost_fn()
				ptfluid.remap("out")[...] *= args.n_plasma/args.npunit*nppin/npp
				# recalculate field
				verr = recalc_field()
				emfield.remap("in")
				
				logger.debug\
				(f"{' 'if irep else '*'}{irun:06d}/{isub:04d}/{irep:02d}({'E0R'[mode]}) verr={verr:6.3e}")
				
				_errv[isub-1, irep] = verr
				if irep>1 and verr < args.epsilon:
					break
			
			# end sub-cycle, collect data to average
			_ptfluid[isub, ...] = ptfluid[...]
			_vplasma[isub, ...] = eq.vmap[...]
			_emenrgy[isub, ...] = np.sum(emfield[..., 1:]**2, axis=2)*0.125/np.pi

		############################################################################
		# end frame cycle
		logger.info(f"end frame ({len(pstore):} samples)")
		
		# acquire frame-avgeraged values
		#print(events.remap("out")[...,0])
		_evtfreq[...] = events.remap("out")[...].astype(np.float32)\
		* (args.nsub*nppin/np_counter) / (args.npunit*args.dt*args.nsub)
		
		# keep ensemble's size constant (if required)
		if args.resample and npp != nppin:
			data, _ = pstore.extract()
			np.random.shuffle(data)
			pstore.reset()
			pstore.inject({"e": data[:min(npp, nppin)]})
			if nppin > npp:
				pstore.inject({"e": data[:nppin-npp]})
			logger.info(f"resample {npp} -> {nppin}")
			# recalc field to preserve integrity
			ppost_fn()
			ptfluid.remap("out")[...] *= args.n_plasma/len(pstore)
		
		############################################################################
		ne = np.nanmean(_ptfluid[..., 0])
		ke = np.nanmean(_ptfluid[..., 1])/ne*2.842815e-16
		vx = np.nanmean(_ptfluid[..., 2])/ne
		vy = np.nanmean(_ptfluid[..., 3])/ne
		
		logger.info(f"ne = {ne:e} cm^-3")
		logger.info(f"vx = {vx:e} cm/s")
		logger.info(f"vy = {vy:e} cm/s")
		logger.info(f"ke = {ke:e} eV")
		
		for j, entry in enumerate(chinfo):
			freq = np.mean(_evtfreq[..., j])/args.n_bgrnd
			logger.info(f"#{j+1:02d} {entry:<40} {freq:e}")
		
		# save frame
		if (fpath := args.save):
			save_frame(f"{fpath}/frame{irun:06d}.zip", "w", **{
			 "cfg" : {**cfg,
			  "tindex": [(irun - 1) * args.nsub, irun * args.nsub],
			 },
			 # plasma potential:
			 "vplasma": np.mean(_vplasma, axis=0),
			 # field-energy
			 "emenrgy": np.mean(_emenrgy, axis=0),
			 # electron concentration, kinetic energy, x/y-fluxes:
			 "ptfluid": np.mean(_ptfluid, axis=0),
			 # event-frequencies:
			 "evtfreq": _evtfreq,
			 "events" : events[...],
			 "bgrnd" : bgrnd[...],
			 # error-vector
			 **({"errv": _errv} if args.nrep else {}),
			 },
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
		"default"  : "DEBUG",
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
	"--extra": {
		"type"     : float,
		"default"  : 2.5,
		"action"   : check_arg(lambda x: x>0, "{} <= 0"),
		"help"     : "extra storage capacity", 
	},
	"--resample" : {
		"help": "resample eVDF each frame to keep size of ensemble constant",
		"action": argparse.BooleanOptionalAction,
	},
	"--dt": {
		"type"     : float,
		"default"  : 2.5e-12,
		"help"     : "simulation's time-step (t->t+dt, s)",
	},
	"--n_plasma"   : {
		"type"     : float,
		"default"  : 1e12,
		"help"     : "plsama concentration, (cm^-3)",
	},
	"--n_bgrnd": {
		"help"     : "sum of backgrounds' concentrations, (1/cm^3)",
		"type"     : float,
		"default"  : 1e17,
		"action"   : check_arg(lambda x: x>0, "should be positive"),
	},
	"--bginfo": {
		"help"     : "list with KEY:FRACTION separated by spaces",
		"nargs"    : "+",
		"type"     : str,
		"required" : True,
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
