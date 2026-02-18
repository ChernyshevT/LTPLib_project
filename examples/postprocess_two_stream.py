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
def get_cmp(frame, key: str):
	return frame.ptfluid[..., frame.cfg.flinfo.index(key)]

funcs = {
  "TIME":   lambda f: 0.5*np.sum(f.cfg.tindex)*f.cfg.dt
, "EMFEN":  lambda f: f.emenrgy*2.99792458e2/8/np.pi
, "C_e":    lambda f: get_cmp(f, "C_e")
, "ENxx_e": lambda f: get_cmp(f, "Pxx_e")/f.C_e * 2.842815e-16 # e^-
, "ENyy_e": lambda f: get_cmp(f, "Pyy_e")/f.C_e * 2.842815e-16
, "ENzz_e": lambda f: get_cmp(f, "Pzz_e")/f.C_e * 2.842815e-16
, "C_i":    lambda f: get_cmp(f, "C_i")
, "ENxx_i": lambda f: get_cmp(f, "Pxx_i")/f.C_i * 5.222763e-13 # H^+
, "ENyy_i": lambda f: get_cmp(f, "Pyy_i")/f.C_i * 5.222763e-13
, "ENzz_i": lambda f: get_cmp(f, "Pzz_i")/f.C_i * 5.222763e-13
, "CHARGE": lambda f: ((f.C_i if f.cfg.ions else 1) - f.C_e)/f.cfg.n_plasma
}

keys = ["TIME", "EMFEN", "ENxx_e", "ENyy_e", "ENzz_e", "ENxx_i", "ENyy_i", "ENzz_i"]

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

