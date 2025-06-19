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
		
		nd = pdata.data.shape[1]-3
		t0,t1 = [frame.cfg.dt*k for k in frame.cfg.tindex]
		
		grads = np.gradient(frame.vplasma, *frame.cfg.step, edge_order=2)
		en_field = np.mean(sum(grad**2 for grad in grads))*2.99792458e2/8/np.pi
		vp_range = np.max(np.abs(frame.vplasma))*2.99792458e2
		
		j0,j1,j2 = pdata.index
		vx = pdata.data[j0:j1, nd+0]
		vy = pdata.data[j0:j1, nd+1]
		vz = pdata.data[j0:j1, nd+2]
		
		key = f"stat{frame.cfg.tindex[1]:06d}"
		dset[key] = {
		 "cfg"      : frame.cfg,
		 "en_exx"   : np.mean(vx*vx)*2.842815e-16,
		 "en_eyy"   : np.mean(vy*vy)*2.842815e-16,
		 "en_ezz"   : np.mean(vz*vz)*2.842815e-16,
		 "en_field" : en_field,
		 "vp_range" : vp_range,
		}
		if frame.cfg.nrep:
			dset[key].update({"verrs" : frame.verrs})
		
		if frame.cfg.ions:
			vx = pdata.data[j1:j2, nd+0]
			vy = pdata.data[j1:j2, nd+1] 
			vz = pdata.data[j1:j2, nd+1] 
			dset[key].update({
			 "en_ixx" : np.mean(vx*vx)*5.182139e-13,
			 "en_iyy" : np.mean(vy*vy)*5.182139e-13,
			 "en_izz" : np.mean(vz*vz)*5.182139e-13,
			})
	
	fpath = os.path.dirname(os.path.abspath(arg))
	save_frame(f"{fpath}/dset.zip", "w", **dset)
	
	
	return 0


if __name__ == '__main__':
	sys.exit(main(sys.argv[1:]))
