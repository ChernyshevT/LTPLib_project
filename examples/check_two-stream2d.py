#!/usr/bin/env python3
import os, sys
import numpy as np

from util.loggers import *
from util.frames  import *

def main(args):
	setup_logging()
	
	dset  = {}
	for arg in args:
		
		pdata, frame = load_frame(arg), load_frame(arg.replace("pdata", "frame"))
		
		nx,ny = frame.cfg.units
		dx,dy = frame.cfg.step
		t0,t1 = [frame.args.dt*k for k in frame.cfg.tindex]
		ng    = frame.args.order
		
		Ex,Ey = np.gradient(frame.vplasma, dx, dy, edge_order=2)
		en_field = np.mean((Ex*Ex + Ey*Ey)/8/np.pi)*300
		vp_range = np.max(np.abs(frame.vplasma))*300
		
		j0,j1,j2 = pdata.index
		xs = pdata.data[j0:j1, 0]
		ys = pdata.data[j0:j1, 1]
		vx = pdata.data[j0:j1, 2]
		vy = pdata.data[j0:j1, 3]
		en_exx = np.mean(vx*vx)*2.842815e-16
		en_eyy = np.mean(vy*vy)*2.842815e-16
		
		dset[arg.replace("pdata","fstat").replace(".zip","")] = {
		 "tindex"   : frame.cfg.tindex,
		 "tstep"    : frame.args.dt,
		 "en_exx"   : en_exx,
		 "en_eyy"   : en_eyy,
		 "en_field" : en_field,
		 "vp_range" : vp_range,
		}
		# ~ if verrs := frame.verrs:
			# ~ dset[arg.replace("pdata", "verrs").replace(".zip","")] = {
			 # ~ "verrs" : verrs,
			# ~ }
		
	
	fpath = os.path.dirname(os.path.abspath(arg))
	save_frame(f"{fpath}/dset.zip", "w", **dset)
	
	
	return 0


if __name__ == '__main__':
	sys.exit(main(sys.argv[1:]))
