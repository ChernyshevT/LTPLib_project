#!/usr/bin/env python3

import sys
import numpy as np
import _ltplib as ltp
from itertools import count, repeat
from util.plots   import *
from util.loggers import *


from test_poisson_eq import show_umap

def main(args):
	
	n, m = 64-1, 32-1
	lx, ly= 1, 0.5

	shape = [n+1, m+1]
	step  = [l/(k-1) for k,l in zip(shape,[lx,ly])]
	yrad  = 0
	
	xs = np.linspace(-lx,+lx, shape[0])
	ys = np.linspace(yrad, yrad+ly, shape[1])
	im_ext = (xs[0],xs[n],ys[0],ys[m])
	
	xs,ys = np.meshgrid(xs,ys, indexing='ij')
	
	_umap = np.zeros(shape, dtype=np.uint8)
	_umap[:n, 0:m] |= ltp.DIFFop("XRT|YCN")
	_umap[1:, 0:m] |= ltp.DIFFop("XLF|YCN")
	
	_vmap = np.zeros(shape, dtype=np.float32)
	
	_vmap[:, 0] = -1;
	_vmap[:, m] = +1;
	
	
	fig = show_umap(_umap)
	plt.show()
	
	
	_cmap =  np.cos(xs/lx*np.pi*2) * np.cos((ys-yrad)/ly*np.pi*2) * 100
	
	# ~ ax.imshow(_vmap)
	# ~ plt.show()
	# ~ show_field(ax, (_vmap, ext), **vopts)
	
	# solver ctor:
	eq = ltp.poisson_eq(_umap, step, radius=yrad)
	eq.vmap[...] = _vmap
	eq.cmap[...] = _cmap
	
	for j, w in enumerate(repeat(1.95), 1): 
		verr = eq.iter(w)
		if verr <= 1e-5 or verr != verr:
			print(f"#{j:06d}: {verr:e}")
			break
	
	fig, ax = mk_subplots([1,12,0.5],[1,6,0.5,0.5], nrows=2, dpi=150)
	show_field(ax[0], (eq.cmap, im_ext), cmap="seismic")
	show_field(ax[1], (eq.vmap, im_ext), cmap="jet")
	plt.show()
	

if __name__ == '__main__':
	setup_logging(level="INFO", root=True)
	logging.getLogger("matplotlib.font_manager").setLevel(logging.WARNING)
	
	sys.exit(main(sys.argv[1:]))
