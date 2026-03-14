#!/usr/bin/env python3
"""
https://jpb911.wixsite.com/landmark/copie-de-test-case-3-fluid-hybrid
"""


from util.frames  import *
from util.loggers import *
from util.args    import *
from itertools    import repeat, count

import  numpy  as np
import _ltplib as ltp

#delete this
from test_poisson_eq import show_umap
from util.plots import *

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
	
	# from table 1
	MLIGHT = ME
	MHEAVY = 7291.712*ME
	TSTEP  = 4e-11 # s
	
	nsub = 1250
	nrun = 10000
	# Nt = nsub * nrun

	Nx = Ny = 256
	dx = dy = 0.01953125 # cm
	
	
	# injection
	R0  = 0.5 # cm
	T0e = 15
	T0i = 0.025
	Ie0 = 20e-3 # A/m
	Ii0 = 8e-3
	
	BZ = 100 # Gauss (1e-2 Teslas)
	
	NINJe = 49.932
	NINJi = 19.973
	WCFFT = 1e5 # CHECK!!
	
	VTERMe = np.sqrt(T0e/STATV_V * 2*ECHARGE/MLIGHT)
	VTERMi = np.sqrt(T0i/STATV_V * 2*ECHARGE/MHEAVY)
	
	
	
	# ~ print(f"{VTERMe=:e} {VTERMi=:e}")
	# ~ print(f"{VTERMe*TSTEP/dx=:f}")
	
	##################################################
	grid = ltp.grid(2, [dx]*2, [range(0, Nx+1, 8)]*2)
	
	#############################################
	pt_cfg = [
	 {"KEY":"e", "CHARGE/MASS": -ECHARGE/MLIGHT},
	 {"KEY":"i", "CHARGE/MASS": +ECHARGE/MHEAVY},
	]
	pstore = ltp.pstore(grid, pt_cfg, capacity = 50000)
	
	#########################################################
	emfield = ltp.vcache(grid, dtype="f32", order=1, vsize=3)
	ppush_fn = ltp.bind_ppush_fn(pstore, "BzExEy:LEAPF", emfield)
	
	##########################################################
	ptfluid  = ltp.vcache(grid, dtype="f32", order=1, vsize=2)
	ppost_fn = ltp.bind_ppost_fn(pstore, "C", ptfluid)
	
	ptfluid2  = ltp.vcache(grid, dtype="f32", order=1, vsize=12)
	ppost_fn2 = ltp.bind_ppost_fn(pstore, "FxFyFzPxxPyyPzz", ptfluid2)
	
	#########################################
	umap = np.zeros([Nx+1]*2, dtype=np.uint8)
	umap[1:-1,1:-1] = ltp.DIFFop("XCN|YCN")
	eq = ltp.poisson_eq(umap, grid.step)
	eq.vmap[...] = 0 #initial approx
	
	# ~ show_umap(umap)
	# ~ plt.show()
	# ~ exit(1)
	
	def recalc_field(w_relax, verr_max):
		
		# gather space charge
		eq.cmap[...] = (ptfluid[..., 0]-ptfluid[..., 1])*M_4PI_E
		
		
		# solve
		iter_max = np.prod(eq.cmap.shape)*2
		for j, w in enumerate(repeat(w_relax), 1): 
			verr = eq.iter(w_relax)
			if verr <= verr_max:
				logger.debug\
				(f"poisson_eq done {verr=:e}, {j} iters")
				break
			if j >= iter_max:
				raise RuntimeError\
				(f"poisson_eq didn't converge after {j} iters (verr = {verr:e})!")
		
		# electric field
		for k, grad in enumerate(np.gradient(eq.vmap, *grid.step), 1):
			emfield[..., k] = -grad
		
	#######################
	spawner = spawner_t(50)
	
	emfield[...,0]  = BZ/CLIGHT
	emfield[...,1:] = 0
	
	# main cycle #################################################################
	for irun in range (1, nrun+1):
		 
		pstore.update_queue(1) # balance computational load
		for isub in range(1, nsub+1):
			
			pstore.inject("e", spawner.generate(NINJe, VTERMe))
			pstore.inject("i", spawner.generate(NINJi, VTERMi))
			
			# sub-sycle
			emfield.remap("in")
			ppush_fn(TSTEP)
			
			ppost_fn()
			ptfluid.remap("out")[...] *= WCFFT*1e2
			
			recalc_field(1.95, 1e-4)
			
			# ~ vmin, vmax = np.min(
			
			logger.debug(f"{irun:06d}/{isub:04d}: {len(pstore)=:09d}")
		
		print("\n",np.sum(ptfluid[..., 0])+np.sum(ptfluid[..., 1]))
		
		save_frame(f"/tmp/frame{irun:06d}.zip"
		, grid = dict(step=grid.step, units=grid.units)
		, cfg = dict(order=1)
		, tindex = [(irun-1)*nsub, irun*nsub]
		, vplasma = eq.vmap*STATV_V
		, ne = ptfluid[..., 0]
		, ni = ptfluid[..., 1]
		)
		
		# ~ cmax = np.max(np.abs(eq.vmap))
		# ~ plt.imshow(eq.vmap
		# ~ , cmap = "seismic"
		# ~ , vmin = -cmax
		# ~ , vmax = +cmax
		# ~ )
		# ~ plt.show()
		
		
			
			
	
################################################################################
if __name__ == "__main__":
	pass
	setup_logging(level="DEBUG", root=True)
	logger = get_logger("problem")
	
	main({}, logger)
