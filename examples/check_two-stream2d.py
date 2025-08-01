#!/usr/bin/env python3
import os, sys
import numpy as np

from util.loggers import *
from util.frames  import *

def main(args):
	setup_logging()
	
	dset  = {}
	for arg in args:
		
		frame = load_frame(arg)
		
		nd = len(frame.cfg.step)
		t0,t1 = [frame.cfg.dt*k for k in frame.cfg.tindex]
		
		grads = np.gradient(frame.vplasma, *frame.cfg.step, edge_order=2)
		en_field = np.mean(sum(grad**2 for grad in grads))*2.99792458e2/8/np.pi
		vp_range = np.max(np.abs(frame.vplasma))*2.99792458e2
		
		slicer = [slice(frame.cfg.order,None,None) for _ in range(nd)]
		
		ne = frame.ne[*slicer]
		ma = ne>0
		pe_xx = frame.pe[*slicer, 0]*2.842815e-16
		pe_yy = frame.pe[*slicer, 1]*2.842815e-16
		pe_zz = frame.pe[*slicer, 2]*2.842815e-16
		
		key = f"stat{frame.cfg.tindex[1]:06d}"
		dset[key] = {
		 "cfg"      : frame.cfg,
		 "en_exx"   : np.mean(pe_xx[ma]/ne[ma]),
		 "en_eyy"   : np.mean(pe_yy[ma]/ne[ma]),
		 "en_ezz"   : np.mean(pe_zz[ma]/ne[ma]),
		 "en_field" : en_field,
		 "vp_range" : vp_range,
		}
		if frame.cfg.nrep:
			dset[key].update({"errv" : frame.errv})
		
		if frame.cfg.ions:
			ni = frame.ni[*slicer]
			ma = ni>0
			pi_xx = frame.pi[*slicer, 0]*5.182139e-13
			pi_yy = frame.pi[*slicer, 1]*5.182139e-13
			pi_zz = frame.pi[*slicer, 2]*5.182139e-13
			dset[key].update({
			 "en_ixx" : np.mean(pi_xx[ma]/ni[ma]),
			 "en_iyy" : np.mean(pi_yy[ma]/ni[ma]),
			 "en_izz" : np.mean(pi_zz[ma]/ni[ma]),
			})
	
	fpath = os.path.dirname(os.path.abspath(arg))
	save_frame(f"{fpath}/dset.zip", "w", **dset)
	
	
	return 0


if __name__ == '__main__':
	sys.exit(main(sys.argv[1:]))
