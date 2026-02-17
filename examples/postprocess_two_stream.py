#!/usr/bin/env python3

import numpy as np
import pandas as pd
import os
import shutil

from util.frames import *
from util.args  import *

import scipy.interpolate as intrp
from itertools import count, takewhile

import logging; logger = logging.getLogger()

################################################################################
class distro_h:
	__slots__ = ("func", "xbins", "ybins", "hist")
	
	def __init__ (m, xbins, ybins):
		
		m.func = None

		m.xbins = xbins
		m.ybins = ybins
		m.hist  = None
	
	def add (m, xs, ys):
		hist,*_ = np.histogram2d(xs, ys, [m.xbins, m.ybins], density=True)
		#print(hist[50,50])
		#print(m.xbins)
		#print(m.ybins)
		if m.hist is not None:
			m.hist += hist
		else:
			m.hist = hist
		return m
	
	def normalize (m):
		m.hist = m.hist/np.max(m.hist)
		
		xb = (m.xbins[0:-1]+m.xbins[1:])*0.5
		yb = (m.ybins[0:-1]+m.ybins[1:])*0.5
		
		m.func = intrp.RegularGridInterpolator((xb, yb), m.hist\
		, bounds_error=False, fill_value=None)
		return m.func
	
	def get (m, xs, ys):
		return np.asarray\
		([m.func(pt) for pt in zip(xs, ys)], dtype=np.float32)

################################################################################
funcs = {
  "TIME":  lambda f:0.5*np.sum(f.cfg.tindex)*f.cfg.dt
, "EMFEN": lambda f: \
   f.emenrgy*2.99792458e2/8/np.pi
, "Ce":    lambda f: \
   f.ptfluid[..., f.cfg.flinfo.index("Ce")]
, "KXXENe":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PXXe")]/f.Ce*2.842815e-16
, "KYYENe":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PYYe")]/f.Ce*2.842815e-16
, "KZZENe":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PZZe")]/f.Ce*2.842815e-16
, "Ci":    lambda f: f.ptfluid[..., f.cfg.flinfo.index("Ci")]
, "KXXENi":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PXXi")]/f.Ci
, "KYYENi":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PYYi")]/f.Ci
, "KZZENi":  lambda f: f.ptfluid[..., f.cfg.flinfo.index("PZZi")]/f.Ci

, "SCHARGE": lambda f: (f.Ci if f.cfg.ions else f.cfg.n_plasma) - f.Ce 
}

keys = ["TIME", "EMFEN", "KXXENe", "KYYENe"]

################################################################################
def main(args):
	
	if os.path.exists(fname:=f"{os.path.abspath(args.fdir)}.dset.zip"):
		if input(f"\"{fname}\" already exists, delete [y|yes]? ") in ["y", "yes"]:
			os.remove(fname)
		else:
			exit(0)
	
	##############################################################################
	stats, cfg, n = [], None, 0
	while os.path.exists(fname:=f"{args.fdir}/frame{n+1:06d}.zip"):
		print(f"\rread \"{fname}\"", end="")
		frame = load_frame(fname).add_funcs(**funcs)
		stats.append({k:np.mean(frame[k]) for k in keys})
		cfg, n = frame.cfg, n+1
	print("\rdone")
	stats = pd.DataFrame(stats)
	print(stats)
	dset = {key: stats[key].to_numpy() for key in keys} | {"cfg" : cfg}
	print(dset.keys())
	save_frame(f"{os.path.abspath(args.fdir)}.dset.zip", **dset)
	
	# ~ fields = ["ENe", "UDRIFTe", "EVENTS/PT"]
	# ~ print(dset.keys())
	# ~ save_frame(f"{os.path.abspath(args.fdir)}.dset.zip", **dset)

	# ~ line = f"| #FRAME| ERRMAX|"\
	     # ~ + "|".join([f"{key:>10}" for key in fields]) + "|"
	# ~ print(f"{line}\n{'-'*len(line)}")
	
	# ~ k_avg = None
	# ~ for j, row in stats.iterrows():
		
		# ~ errmx = None
		# ~ if j >= args.window:
			# ~ a, b = j-args.window+1, j+1
			# ~ for field in fields:
				# ~ err = np.std(stats[field][a:b])/np.mean(stats[field][a:b])
				# ~ if errmx is not None:
					# ~ errmx = max(abs(err), errmx)
				# ~ else:
					# ~ errmx = abs(err)

		# ~ line = f"| {j+1:>6d}"\
		     # ~ + (f"| {errmx*1e2:>5.2f}%|" if errmx else "| ----- |")\
		     # ~ +  "|".join([f"{row[key]:>10.3e}" for key in fields]) + "|"
		# ~ print(f"{line}")

		# ~ if k_avg is None and errmx is not None and errmx < args.precision:
			# ~ k_avg = j
			# ~ print('-'*len(line))
		
		# ~ if k_avg is not None and errmx is not None and errmx >= args.precision:
			# ~ raise RuntimeError(f"errmx = {errmx:e} >= {args.precision:e}!")
	
	# ~ # endline ##########
	# ~ print('-'*len(line))
	
	# ~ ###################################
	# ~ dset = {"cfg": vars(frame.cfg), "avg":{}}
	# ~ for key in keys:
		# ~ entry = np.stack(stats[k_avg:][key])
		# ~ dset["avg"][key] = entry.mean(axis=0), entry.std(axis=0)
	
	# ~ #############
	# ~ if args.eVDFxy:
		
		# ~ count,dist,pdata = 0, None, None
		# ~ for k in range(k_avg, n+1):
			# ~ if os.path.exists(fname:=f"{args.fdir}/pdata{k:06d}.zip"):
				# ~ pdata = load_frame(fname).data
				# ~ vxs,vys,*_ = pdata.T[2:]
				
				# ~ vmax = np.max(np.sqrt(vxs*vxs + vys*vys))
				# ~ if dist is None:
					# ~ xbins, ybins = [np.linspace(-vmax, +vmax, 256) for _ in range(2)]
					# ~ dist = distro_h(xbins, ybins)
				
				# ~ print(f"add \"{fname}\": vmax = {vmax:e}")
				# ~ dist.add(vxs, vys)
				# ~ count += 1
		
		# ~ if count < 1:
			# ~ raise RuntimeError("no pdata-files were found!")
		# ~ else:
			# ~ np.random.shuffle(pdata)
			# ~ dset["vbins"] = np.asarray([dist.xbins, dist.ybins])
			# ~ dset["vpart"] = pdata[:,2:]
			# ~ dset["VDFxy"] = dist.hist/count
	
	# ~ ############################################################
	# ~ print(dset.keys())
	# ~ save_frame(f"{os.path.abspath(args.fdir)}.dset.zip", **dset)
	
################################################################################

args = {
 "--fdir" : {
  "type": str,
  "required": True
 },
}

if __name__ == '__main__':
	import sys
	from util import setup_logging;
	
	setup_logging()
	args = parse_args(sys.argv, args)
	
	try:
		sys.exit(main(args))
	except KeyboardInterrupt:
		print("manual exit")
		sys.exit(0)

