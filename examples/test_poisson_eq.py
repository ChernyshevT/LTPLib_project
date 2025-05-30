#!/usr/bin/env python3

import sys
import numpy as np
import _ltplib as ltp
from itertools import count
from util.plots   import *
from util.loggers import *

def laplace(vmap, step):
	cmap = np.zeros_like(vmap)

	for ax in range(len(vmap.shape)):
		diff = np.copy(vmap)
		for k in [1,2]:
			diff = np.gradient(diff, step[ax], axis=ax, edge_order=2)
		cmap[...] += diff
	
	return cmap

def main(args):
	pass
	
	ltp.load_backend("default")
	
	lx,ly = 1.5,1.5
	nx,ny = 128,63
	
	ytype = 2
	
	shape = (nx+1,ny+1)
	step  = [l/(k-1) for k,l in zip(shape,[lx,ly])]
	
	# create array to store the voltage
	_vmap = np.zeros(shape, dtype=np.float32)
	# set-up test distribution
	xs,ys = np.meshgrid(*[np.linspace(-l/2,l/2,n) for n,l in zip(shape,[lx,ly])], indexing='ij')
	_vmap[...] =  np.cos(xs*np.pi*2) * np.cos(ys/ly*np.pi*4) + xs/lx
	
	U = ltp.poisson_eq.uTYPE
	# create array to encode finite differences:
	umap = np.zeros(shape, dtype=np.uint8)
	# set central differences for x and y axes:
	umap[... ]    = U.XCN|U.YCN
	# set Dirichlet boundary for left and right edges
	umap[0, :] = U.VAL|U.XRT
	umap[nx,:] = U.VAL|U.XLF
	
	if   ytype==2:
		pass
	elif ytype==1:
		# set open boundary for lower and upper edges
		umap[1:nx, 0] = U.XCN|U.YRT
		umap[1:nx,ny] = U.XCN|U.YLF
	else:
		# set Dirichlet boundary for lower and upper edges
		umap[1:nx, 0] = U.VAL; _vmap[1:nx, 0] = 0
		umap[1:nx,ny] = U.VAL; _vmap[1:nx,ny] = 0
	
	
	# create and fill array with charge-densities
	_cmap = laplace(_vmap, step)
	
	# solver ctor:
	eq = ltp.poisson_eq(umap, step)
	
	eq.cmap[...] = _cmap
	eq.vmap[...] = _vmap
	
	#reset values
	if ytype:
		eq.vmap[1:nx, :] = 0
	else:
		eq.vmap[1:nx, 1:ny] = 0
		

	
	for k in range(25):
		# SOR-iterations
		if ytype:
			eq.cmap[1:nx, :] += np.random.normal(size=[nx-1,ny+1], scale=0)
		else:
			eq.cmap[1:nx, 1:ny] += np.random.normal(size=[nx-1,ny-1], scale=0)
		
		
		nmax = np.prod(eq.vmap.shape*2)
		for j in count(1): 
			verr = eq.iter(1.5)
			if verr <= 1e-5 or verr != verr:
				print(f"#{j:06d}: {verr:e}, {np.sum(eq.cmap):e}")
				break
			if j >= nmax:
				print(f"#{j:06d}: {verr:e} (fail to converge!)")
				break
		pass
	
	#calculate corresponding charge-density:
	cmap = laplace(eq.vmap, step)
	
	##############################################################################
	fig,axs = mk_subplots([0.25,5,0.25,0.25],[0.25,5,0.75,0.75]
	, ncols=3, nrows=2, dpi=200, sharex="all", sharey="all")
	for ax in axs.flat:
		ax.set_xticks([])
		ax.set_yticks([])
		
	vlim,clim = np.max(np.abs(_vmap)),np.max(np.abs(_cmap))
	vopts = {"vmin":-vlim,"vmax":+vlim,"cmap":"jet"} 
	copts = {"vmin":-clim,"vmax":+clim,"cmap":"seismic"}
	
	
	ext = (-lx/2,lx/2,-ly/2,ly/2)
	axs[0].set_title(r"$\psi_{\rm orig}$", loc="left")
	show_field(axs[0], (_vmap, ext), **vopts)
	axs[1].set_title(r"$\psi_{\rm calc}\ \leftarrow\ q_{\rm orig} \pm \delta q_{\rm noise}$", loc="left")
	show_field(axs[1], (eq.vmap, ext), **vopts)
	axs[2].set_title(r"$\psi_{\rm calc}-\psi_{\rm orig}$", loc="left")
	show_field(axs[2], (eq.vmap-_vmap, ext), **vopts)
	
	
	axs[3].set_title(r"$q_{\rm orig}=\nabla^2\psi_{\rm orig}$", loc="left")
	show_field(axs[3], (_cmap, ext), **copts)
	axs[4].set_title(r"$q_{\rm orig}\pm\delta q$", loc="left")
	show_field(axs[4], (eq.cmap, ext), **copts)
	axs[5].set_title(r"$\nabla^2\psi_{\rm calc}-\left(q_{\rm orig}\pm\delta q_{\rm noise}\right)$", loc="left")
	
	show_field(axs[5], (laplace(eq.vmap, step)-eq.cmap, ext), **copts)
	
	plt.show()
	
if __name__ == '__main__':
	setup_logging(level="INFO", root=True)
	
	# ~ logging.getLogger("matplotlib.font_manager").setLevel(logging.WARNING)
	
	sys.exit(main(sys.argv[1:]))
