#!/usr/bin/env python3
import sys
import numpy as np

from util.loggers import *
from util.frames  import *

def main(args):
	setup_logging()
	
	for arg in args:
		
		pdata, frame = load_frame(arg), load_frame(arg.replace("pdata", "frame"))
		
		nx,ny = frame.cfg.units
		dx,dy = frame.cfg.step
		t0,t1 = [frame.args.dt*k for k in frame.cfg.tindex]
		ng    = frame.args.order
		
		nskip = 10
		xs = pdata.data[::nskip, 0]
		ys = pdata.data[::nskip, 1]
		vx = pdata.data[::nskip, 2]
		vy = pdata.data[::nskip, 3]
		pxx = np.mean(vx*vx)*2.842815e-16
		pyy = np.mean(vy*vy)*2.842815e-16
		
		ex,ey  = np.gradient(frame.vplasma, dx, dy, edge_order=2)
		venrgy = np.mean((ex*ex + ey*ey)/8/np.pi)*300
		vrange = np.max(np.abs(frame.vplasma))*300

		print(f"{t1*1e9:07.3f}ns: pe_xx, p_yy: {pxx:e} {pyy:e}, fen = {venrgy:e} eV, {vrange:e} V")
	return 0


if __name__ == '__main__':
	sys.exit(main(sys.argv[1:]))
