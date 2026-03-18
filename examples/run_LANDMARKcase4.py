#!/usr/bin/env python3
"""
https://jpb911.wixsite.com/landmark/copie-de-test-case-3-fluid-hybrid
"""

import sys, os, timeit, shutil, signal

from util.frames  import *
from util.loggers import *
from util.args    import *
from datetime     import datetime
from time         import time

import  numpy  as np
import _ltplib as ltp

#delete this
from test_poisson_eq import show_umap
from util.plots import *

class poisson_eq_SOR(ltp.poisson_eq):
	__slots__ = ("vmap_copy")
	
	def __init__ (eq, grid):
		umap = np.zeros([nx+1 for nx in grid.units], dtype=np.uint8)
		umap[1:-1,1:-1] = ltp.DIFFop("XCN|YCN")
		# ltp.poisson_eq ctor
		super().__init__(umap, grid.step)
		
		# initial approx
		eq.vmap[...] = 0
		eq.vmap_copy = eq.vmap.copy()
	
	def solve(eq, w_relax, verr_max=None, _debug=False):
		
		eq.vmap_copy[...] = eq.vmap[...]
		
		if verr_max is None:
			vmin = np.min(eq.vmap)
			vmax = np.max(eq.vmap)
			vres = np.finfo(np.float32).resolution
			verr_max = (vmax - vmin)*vres
			if verr_max < vres:
				verr_max = vres
		
		verr, n = super().solve(w_relax, verr_max)
		if _debug:
			logger.debug(f" poisson_eq done {verr=:e}, {n} iters")
		return verr

class spawner_t:
	__slots__ = ("parts", "radii", "alpha")
	
	def __init__ (self, size):
		self.parts = np.empty([size, 5], dtype=np.float32)
		self.radii = np.empty([size], dtype=np.float32)
		self.alpha = np.empty([size], dtype=np.float32)
		
	def generate(self, npp, VTERM):
		npp = int(npp + np.random.uniform(0., 1.))
		
		if npp > self.parts.shape[0]:
			self.parts = np.empty([npp, 5], dtype=np.float32)
			self.radii = np.empty([npp], dtype=np.float32)
			self.alpha = np.empty([npp], dtype=np.float32)
		
		self.radii[:npp] = np.random.triangular(0, 0.5, 0.5, [npp])
		self.alpha[:npp] = np.random.uniform(0, 2*np.pi, [npp])
		
		self.parts[:npp, 0 ] = 2.5+self.radii[:npp]*np.cos(self.alpha[:npp])
		self.parts[:npp, 1 ] = 2.5+self.radii[:npp]*np.sin(self.alpha[:npp])
		self.parts[:npp, 2:] = np.random.normal(0, VTERM, [npp, 3])
		
		return self.parts[:npp, :]

########################
def main (args, logger):
	
	ltp.load_backend("default")
	
	STATV_V = 2.997924e+02
	ME      = 9.109383e-28 # gram
	ECHARGE = 4.803204e-10 # statC
	M_4PI_E = 6.035884e-09
	CLIGHT  = 2.997924e+10 # cm/s
	
	MLIGHT = ME
	MHEAVY = 7291.712*ME #helium
	TSTEP  = 4e-11 # s
	
	nsub = 1250
	nrun = 10000
	# Nt = nsub * nrun

	nx = ny = 256
	dx = dy = 0.01953125 # cm
	
	
	# injection
	R0  = 0.5   # cm
	T0e = 15.0  # eV
	T0i = 0.025 # eV
	Ie0 = 20e-3 # A/m
	Ii0 = 8e-3 
	
	BZ = 100 # Gauss (1e-2 Teslas)
	
	NINJe = 49.932
	NINJi = 19.973
	WCFFT = 1e5 *1e2 # CHECK!!
	
	VTERMe = np.sqrt(T0e/STATV_V * 2*ECHARGE/MLIGHT)
	VTERMi = np.sqrt(T0i/STATV_V * 2*ECHARGE/MHEAVY)
	
	##################################################
	grid = ltp.grid(2, [dx]*2, [range(0, nx+1, 8)]*2)
	
	# ~ print(grid.flags)
	
	#############################################
	pt_cfg = [
	 {"KEY":"e", "CHARGE/MASS": -ECHARGE/MLIGHT},
	 {"KEY":"i", "CHARGE/MASS": +ECHARGE/MHEAVY},
	]
	pstore = ltp.pstore(grid, pt_cfg, capacity = 500000, vsize=6)
	
	#########################################################
	emfield = ltp.vcache(grid, dtype="f32", order=1, vsize=3)
	ppush_fns = [ltp.bind_ppush_fn (pstore, f"BzExEy:{mover}", emfield) \
	 for mover in ["LEAPF"]
	]

	##########################################################
	ptfluid1  = ltp.vcache(grid, dtype="f32", order=1, vsize=2)
	ppost1_fn = ltp.bind_ppost_fn(pstore, "C", ptfluid1)
	
	ptfluid2  = ltp.vcache(grid, dtype="f32", order=1, vsize=12)
	ppost2_fn = ltp.bind_ppost_fn(pstore, "FxFyFzPxxPyyPzz", ptfluid2)
	
	#########################################
	eq = poisson_eq_SOR(grid)
	
	def recalc_field(w_relax, verr_max=None, _debug=False):
		
		# gather space charge
		eq.cmap[...] = (ptfluid1[..., 0] - ptfluid1[..., 1])*M_4PI_E
		
		# solve
		vdiff = eq.solve(w_relax, verr_max, _debug)
		
		# electric field
		for k, grad in enumerate(np.gradient(eq.vmap, *grid.step), 1):
			emfield[..., k] = -grad
		
		return vdiff

	if fpath := args.load:
		ptframe = load_frame(fpath)
		for key,a,b in zip(ptframe.descr, ptframe.index, ptframe.index[1:]):
			pstore.inject(key, ptframe.data[a:b])
		logger.info(f"loaded \"{fpath}\" ({len(pstore)} samples injected)")

	#######################
	spawner = spawner_t(50)
	emfield[..., 0]  = BZ/CLIGHT
	emfield[..., 1:] = 0
	
	# reserve backup
	pdata, pindex = None, None
	
	_debug = ("DEBUG" == args.loglevel)

	##############################################################################
	if args.run == False or not (args.run or input(f"run? [y] ") == "y"):
		exit(0)

	############
	# main cycle
	for irun in range (args.nstart, args.nrun+1):
		
		dtime, dnpts = 0, 0
		
		if args.reserve_backup:
			pdata, pindex = pstore.extract()

		# start new frame [t --> t + TSTEP*args.nsub]
		t0, t1 = (irun-1)*args.nsub*TSTEP, irun*args.nsub*TSTEP
		logger.info(f"frame#{irun:06d} ({t0*1e6:07.3f} -> {t1*1e6:07.3f} us)..")
		try:
			for isub in range(1, args.nsub+1):
			
				pstore.inject("e", spawner.generate(NINJe, VTERMe))
				pstore.inject("i", spawner.generate(NINJi, VTERMi))
				
				# sub-cycle for semi-implicit solver
				for irep in range(0, args.nrep+1):
					mode = (args.nrep>0)+(irep>0)
					
					t0 = time()
					emfield.remap("in")
					ppush_fns[mode](TSTEP)
					
					ppost1_fn()
					ptfluid1.remap("out")[...] *= WCFFT
					dtime += time()-t0
					dnpts += len(pstore)
				
					vdiff = recalc_field(1.95, verr_max=1e-2/STATV_V, _debug=_debug)*STATV_V
					
					if _debug:
						logger.debug\
						(f"{' 'if irep else '*'}{irun:06d}/{isub:04d}/{irep:02d}({'E0R'[mode]}) {vdiff=:6.3e} V")
					
					if irep and vdiff < args.epsilon:
						break
			
		except Exception as e:
			logger.critical(f"#{irun:06d}/{isub:04d}/{irep:02d}: {e}")
			
			if (fpath := args.save) and args.reserve_backup:
				save_frame(f"{fpath}/pdata-backup{irun:06d}.zip", "w"
				, data = pdata
				, index = pindex
				, descr = pstore.ptlist
				)
			
			return 1
		# end frame
		
		logger.info(f"frame#{irun:06d} done, {len(pstore)=} pts ({dtime/dnpts*1e9:f} ns/pt)");

		# balance computational load
		pstore.update_queue(1)
		n = 0
		for k,v in zip(*pstore.queue):
			if 0 == v:
				break
			else:
				n+=1
		logger.info(f"pstore.queue {n} nonempty pools")

		# gather fluxes \& pressures
		ppost2_fn()
		ptfluid2.remap("out")[...] *= WCFFT
		
		# save frame
		if fpath := args.save:
			save_frame(f"{fpath}/frame{irun:06d}.zip", "w"
			, cfg  = dict(order=1, dt=TSTEP)
			, grid = dict(step=grid.step, units=grid.units)
			, tindex = [irun-1, irun]
			, VP = eq.vmap*STATV_V
			, Ne = ptfluid1[..., 0]
			, Ni = ptfluid1[..., 1]
			, Fe = ptfluid2[..., 0:3]
			, Fi = ptfluid2[..., 6:9]
			, Pe = ptfluid2[..., 3:6]
			, Pi = ptfluid2[..., 9:12]
			)
		# save dump
		if (fpath := args.save) and irun in args.dump:
			pdata, pindex = pstore.extract()
			save_frame(f"{fpath}/pdata{irun:06d}.zip", "w"
			, data = pdata
			, index = pindex
			, descr = pstore.ptlist
			)
		
	return 0
	
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
	"--load"     : {
		"type"     : str,
		"required" : False,
		"help"     : "path to load pVDF dump",
	},
	"--nstart" : {
		"type"     : int,
		"default"  : 1,
	},
	"--dump"     : {
		"type"     : int,
		"required" : False,
		"default"  : [],
		"nargs"    : "+",
		"help"     : "frame list to writing pVDF dumps",
	},
	"--reserve-backup"  : {
		"action"   : argparse.BooleanOptionalAction,
		"help"     : "extract the samples befor the start of each frame, make save if error"
	},
	"--nrun": {
		"type": int,
		"default": 10000,
		"help": "number of frames to calculate (t->t+dt*nsub)"
	},
	"--nsub": {
		"type": int,
		"default": 1250, #2
		"help": "number of sub-steps per frame (t->t+dt)"
	},
	"--nrep"    : {
		"type"    : int,
		"default" : 0,
		"help"    : "number of corrector runs (0 for explicit, >=1 for semi-implicit)"
	},
	"--epsilon" : {
		"type"    : float,
		"default" : 0,
		"help"    : "epsilon to stop semi-implicit solver earlier (V)"
	},
}
################################################################################
def handle_sigterm(n, frame):
	raise SystemExit(signal.strsignal(n))

if __name__ == "__main__":
	signal.signal(signal.SIGTERM, handle_sigterm)
	
	args = parse_args(sys.argv, args)
	
	setup_logging(level=args.loglevel.upper(), root=True)
	logger = get_logger("problem")
	
	logger.info(f"{args=}")
	
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
		
		if args.reserve_backup and not args.save:
			raise ValueError("can not make backup without save dir")
			
		if args.dump and not args.save:
			raise ValueError("can not save samples without save dir")
		
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
		logger.exception(err)
		sys.exit(1)
