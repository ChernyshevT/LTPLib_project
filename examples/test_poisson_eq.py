#!/usr/bin/env python3

import sys
import numpy as np
import _ltplib as ltp
from itertools import count
from util.plots import *

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
	nx,ny = 384,384
	
	shape = (nx+1,ny+1)
	step  = [l/(k-1) for k,l in zip(shape,[lx,ly])]
	
	# create array to store the voltage
	vmap = np.zeros(shape, dtype=np.float32)
	# set-up test distribution
	xs,ys = np.meshgrid(*[np.linspace(-l/2,l/2,n) for n,l in zip(shape,[lx,ly])], indexing='ij')
	vmap[...] =  np.cos(xs*np.pi*2)*np.sin(ys*np.pi*2)*1 + xs/lx
	
	U = ltp.poisson_eq.uTYPE
	# create array to encode finite differences:
	umap = np.zeros(shape, dtype=np.uint8)
	# set central differences for x and y axes:
	umap[... ]    = U.XCN|U.YCN
	# set Dirichlet boundary for left and right edges
	umap[0, :] = U.VAL
	umap[nx,:] = U.VAL
	# set Dirichlet boundary for lower and upper edges
	# ~ umap[1:nx, 0] = U.VAL; vmap[1:nx, 0] = 0
	# ~ umap[1:nx,ny] = U.VAL; vmap[1:nx,ny] = 0
	# set open boundary for lower and upper edges
	umap[1:nx, 0] = U.XCN|U.YRT
	umap[1:nx,ny] = U.XCN|U.YLF

	# back-up voltage
	_vmap = np.copy(vmap)	
	# create and fill array with charge-densities
	_cmap = laplace(_vmap, step)
	
	# solver ctor:
	eq = ltp.poisson_eq(step, umap, _cmap, vmap)
	
	
	for k in range(80):
		# fill original vmap with garpage to test the solver:
		vmap[1:nx, 1:ny] += np.random.normal(size=[nx-1,ny-1], scale=0.01)
		# SOR-iterations
		for j in count(): 
			verr = eq.iter(1.95)
			print(f"\r#{j:06d}: {verr:e}",end="")
			if verr <= 1e-4 or verr != verr:
				print("")
				break
			if j >= np.prod(vmap.shape):
				print(" fail to converge!")
				break
	
	#calculate corresponding charge-density:
	cmap = laplace(vmap, step)
	
	##############################################################################
	fig,axs = mk_subplots([0.25,5,0.25,0.25],[0.25,5,0.75,0.75]
	, ncols=2, nrows=2, dpi=200, sharex="all", sharey="all")
	for ax in axs.flat:
		ax.set_xticks([])
		ax.set_yticks([])
		
	vlim,clim = np.max(np.abs(_vmap)),np.max(np.abs(_cmap))
	vopts = {"vmin":-vlim,"vmax":+vlim,"cmap":"jet"} 
	copts = {"vmin":-clim,"vmax":+clim,"cmap":"seismic"}
	
	
	ext = (-lx/2,lx/2,-ly/2,ly/2)
	axs[0].set_title(r"$\psi_{\rm orig}$", loc="left")
	show_field(axs[0], (_vmap, ext), **vopts)
	axs[1].set_title(r"$\nabla^2\psi_{\rm orig}$", loc="left")
	show_field(axs[1], (_cmap, ext), **copts)
	
	axs[2].set_title(r"$\psi_{\rm calc}\ \leftarrow\ \nabla^2\psi_{\rm orig}$", loc="left")
	show_field(axs[2], (vmap, ext), **vopts)
	axs[3].set_title(r"$\nabla^2\psi_{\rm calc}$", loc="left")
	show_field(axs[3], (cmap, ext), **copts)
	plt.show()
	
if __name__ == '__main__':
	sys.exit(main(sys.argv[1:]))
