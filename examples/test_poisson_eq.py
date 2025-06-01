#!/usr/bin/env python3

import sys
import numpy as np
import _ltplib as ltp
from itertools import count, repeat
from util.plots   import *
from util.loggers import *

def show_umap(umap):
	U = ltp.poisson_eq.uTYPE
	
	fig,ax = mk_subplots([0,8,2.75],[0,8,0], dpi=200)
	
	ax.set_xlim(-0.5, umap.shape[0]-0.5)
	ax.set_ylim(-0.5, umap.shape[1]-0.5)
	ax.axis("off"),
	
	xlfs, xrts, xcns, ylfs, yrts, ycns, vals = [],[],[],[],[],[],[]
	for x in range(umap.shape[0]):
		for y in range(umap.shape[1]):
			ucode = umap[x,y]
			if ucode == U.VALUE:
				vals.append((x,y))
			else:
				match ucode & U.XCENTER:
					case U.XLFOPEN:
						xlfs.append((x,y))
					case U.XRTOPEN:
						xrts.append((x,y))
					case U.XCENTER:
						xcns.append((x,y))
				match ucode & U.YCENTER:
					case U.YLFOPEN:
						ylfs.append((x,y))
					case U.YRTOPEN:
						yrts.append((x,y))
					case U.YCENTER:
						ycns.append((x,y))
	
	############################################################
	ax.plot(*zip(*vals), ls="", c="k", marker="s", markersize=2, label=r"{\tt VALUE}")
	ax.plot(*zip(*xlfs), ls="", c="k", marker=0,   markersize=4, label=r"{\tt XLFOPEN}")
	ax.plot(*zip(*xrts), ls="", c="k", marker=1,   markersize=4, label=r"{\tt XRTOPEN}")
	ax.plot(*zip(*xcns), ls="", c="k", marker="_", markersize=8, label=r"{\tt XCENTER}")
	ax.plot(*zip(*ylfs), ls="", c="k", marker=3,   markersize=4, label=r"{\tt YLFOPEN}")
	ax.plot(*zip(*yrts), ls="", c="k", marker=2,   markersize=4, label=r"{\tt YRTOPEN}")
	ax.plot(*zip(*ycns), ls="", c="k", marker="|", markersize=8, label=r"{\tt YCENTER}")
	
	legend_conf = {
		"bbox_to_anchor":(1,0,1,1),
		"fontsize":10,
		"borderaxespad":0,
		"columnspacing":0.25,
		"labelspacing" :1,
		"frameon":0,
		"handlelength":.75,
		"loc":"center left",
		"ncol":1,
	}
	ax.legend(**legend_conf)
	
	return fig;


def laplace(vmap, step):
	cmap = np.zeros_like(vmap)

	for ax in range(len(vmap.shape)):
		diff = np.copy(vmap)
		for k in [1,2]:
			diff = np.gradient(diff, step[ax], axis=ax, edge_order=2)
		cmap[...] += diff
	
	return cmap
	
def w_chebyshev(umap, w=1.0):
	"""
	Generator for Chebyshev-acceleration w_relax
	"""
	U = ltp.poisson_eq.uTYPE
	N = np.sum((umap.flat != 0) & (umap.flat != U.VALUE), dtype=int)
	p = (1.0 - 0.5*np.pi**2/N**2)**2
	
	while 1:
		yield w
		w = 1.0/(1.0 - 0.25*p*w)

def main(args):
	pass
	
	ltp.load_backend("default")
	
	lx,ly = 1.5,1.5
	nx,ny = 191,191
	
	noise_lvl = 1.
	
	shape = (nx+1,ny+1)
	step  = [l/(k-1) for k,l in zip(shape,[lx,ly])]
	
	# create array to store the voltage
	_vmap = np.zeros(shape, dtype=np.float32)
	# set-up test distribution
	xs,ys = np.meshgrid(*[np.linspace(-l/2,l/2,n) for n,l in zip(shape,[lx,ly])], indexing='ij')
	_vmap[...] =  np.cos(xs*np.pi*4) * np.cos(ys/ly*np.pi*4) #+ xs/lx
	
	U = ltp.poisson_eq.uTYPE
	# create array to encode finite differences:
	umap = np.zeros(shape, dtype=np.uint8)
	# set central differences for x and y axes:
	# ~ umap[... ]    = U.XCN|U.YCN
	# ~ # set Dirichlet boundary for left and right edges
	# ~ umap[0, :] = U.VAL
	# ~ umap[nx,:] = U.VAL
	
	umap[0, :]   = umap[0, :]   | U.XRTOPEN 
	umap[nx,:]   = umap[nx,:]   | U.XLFOPEN
	umap[1:nx,:] = umap[1:nx,:] | U.XCENTER
	umap[:, 0]   = umap[:, 0]   | U.YRTOPEN
	umap[:,ny]   = umap[:,ny]   | U.YLFOPEN
	umap[:,1:ny] = umap[:,1:ny] | U.YCENTER
	# ~ umap[...] = U.XCENTER|U.YCENTER
	# ~ umap[nx//2,ny//2] = U.VALUE
	
	r = 0.5**2
	umap[xs**2 + ys**2 < r**2] = U.VALUE
	_vmap[xs**2 + ys**2 < r**2] = 0
	
	# ~ for j, w in enumerate(w_chebyshev(umap)):
		# ~ print(j, w)
		# ~ if j>100:
			# ~ exit()
		
	# ~ fig = show_umap(umap)
	# ~ fig.name = "../docs/imgs/umap_example"
	# ~ save_fig(fig, dpi=200, fmt="png")
	# ~ exit()
	
	# create and fill array with charge-densities
	_cmap = laplace(_vmap, step)
	
	# solver ctor:
	eq = ltp.poisson_eq(umap, step)
	
	eq.cmap[...] = _cmap
	eq.vmap[...] = _vmap
	
	#reset values
	mask = eq.umap!=U.VALUE
	eq.vmap[mask] = 0
	
	for k in range(10):
		# SOR-iterations
		if noise_lvl>0:
			noise = np.random.normal(size=eq.vmap.shape, scale=noise_lvl)
			eq.cmap[mask] += noise[mask]
		
		#https://crunchingnumbers.live/2017/07/09/iterative-methods-part-2/
		
		for j, w in enumerate(repeat(1.975), 1): 
			verr = eq.iter(w)
			print(f"\r#{j:06d}: {verr:e}..", end="")
			if verr <= 1e-5 or verr != verr:
				print(" done!")
				break
			# ~ if j >= nmax:
				# ~ print(f"#{j:06d}: {verr:e} (fail to converge!)")
				# ~ break
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
	axs[4].set_title(r"$q_{\rm orig}\pm\delta q_{\rm noise}$", loc="left")
	show_field(axs[4], (eq.cmap, ext), **copts)
	axs[5].set_title(r"$\nabla^2\psi_{\rm calc}-\left(q_{\rm orig}\pm\delta q_{\rm noise}\right)$", loc="left")
	
	show_field(axs[5], (laplace(eq.vmap, step)-eq.cmap, ext), **copts)
	
	plt.show()
	
if __name__ == '__main__':
	setup_logging(level="INFO", root=True)
	
	# ~ logging.getLogger("matplotlib.font_manager").setLevel(logging.WARNING)
	
	sys.exit(main(sys.argv[1:]))
