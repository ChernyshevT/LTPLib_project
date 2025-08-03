#!/usr/bin/env python3

import numpy as np
import pandas as pd
import os

from util.frames import *
from util.plots  import *
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
		if m.hist is not None:
			m.hist += hist
		else:
			m.hist = hist
	
	def normalize (m):
		m.hist = m.hist/np.max(m.hist)
		
		xb = (m.xbins[0:-1]+m.xbins[1:])*0.5
		yb = (m.ybins[0:-1]+m.ybins[1:])*0.5
		
		m.func = intrp.RegularGridInterpolator((xb, yb), m.hist\
		, bounds_error=False, fill_value=None)
		return m.func
	
	def get (m, xs, ys):
		return np.asarray\
		([m.func(pt) for pt in zip(xs, np.log10(ys))], dtype=np.float32)



################################################################################

funcs = {
"TIME": lambda f:\
 0.5*np.sum(f.cfg.tindex)*f.cfg.dt * 1e9,

"ENe": lambda f:\
 np.mean(f.ptfluid[...,1]/f.ptfluid[..., 0])*2.842815e-16,

"UXe": lambda f:\
 np.mean(f.ptfluid[...,2]/f.ptfluid[..., 0]),

"UYe": lambda f:\
 np.mean(f.ptfluid[...,3]/f.ptfluid[..., 0]),

"UDRIFTe": lambda f:\
 np.sqrt(f.UXe**2 + f.UYe**2),

"EVENTS/PT": lambda f:\
 np.mean(np.sum(f.events, axis=-1))/f.cfg.npunit/f.cfg.nsub,
 
"CFREQ": lambda f:\
 np.mean(f.evtfreq/f.cfg.n_bgrnd, axis=tuple(range(f.evtfreq.ndim-1))),
 
"EMF/EN": lambda f:\
 np.mean(f.emenrgy),
}

keys = ["ENe","UDRIFTe","UXe","UYe","CFREQ"]

def main(args):
	
	stats = []
	n = 0
	while os.path.exists(fname:=f"{args.fdir}/frame{n+1:06d}.zip"):
		frame = load_frame(fname).add_funcs(**funcs)
		stats.append({k:frame[k] for k in funcs})
		n = n+1
		
	stats = pd.DataFrame(stats)
	
	fields = ["ENe", "UDRIFTe", "EVENTS/PT"]
	window, precision = 5, 0.01

	line = f"| #FRAME| ERRMAX|"\
	     + "|".join([f"{key:>10}" for key in fields]) + "|"
	print(f"{line}\n{'-'*len(line)}")
	
	k_avg = None
	for j, row in stats.iterrows():
		
		errmx = None
		if j >= window:
			a, b = j-window+1, j+1
			for field in fields:
				err = np.std(stats[field][a:b])/np.mean(stats[field][a:b])
				if errmx:
					errmx = max(abs(err), errmx)
				else:
					errmx = abs(err)

		line = f"| {j+1:>6d}"\
		     + (f"| {errmx*1e2:>5.2f}%|" if errmx else "| ----- |")\
		     +  "|".join([f"{row[key]:>10.3e}" for key in fields]) + "|"
		print(f"{line}")

		if k_avg is None and errmx is not None and errmx < precision:
			k_avg = j
			print('-'*len(line))
	print('-'*len(line))
	
	################
	dset = {"cfg": frame.cfg, "avg":{}}
	for key in keys:
		entry = np.stack(stats[k_avg:][key])
		dset["avg"][key] = entry.mean(axis=0), entry.std(axis=0)
	
	##############
	if args.eEDF:
		bins = np.logspace(-5,10,15*8+1 ,base=2)
		hist = None
		numb = 0
		
		for k in range(k_avg, n+1):
			if os.path.exists(fname:=f"{args.fdir}/pdata{k:06d}.zip"):
				pdata  = load_frame(fname)
				evs    = np.sum(pdata.data[...,-3:]**2, axis=1)*2.842815e-16
				F00,*_ = np.histogram(evs, bins, density=True)
				if hist is None:
					hist  = F00
				else:
					hist += F00
				numb += 1
		dset["eEDF"]\
		= np.stack([[(a+b)/2 for a,b in zip(bins,bins[1:])], hist/numb])
	
	save_frame(f"{os.path.abspath(args.fdir)}.dset.zip", **dset)
	
################################################################################

args = {
 "--fdir" : {
  "type": str,
  "required": True
 },
 "--eEDF" : {
  "action": argparse.BooleanOptionalAction,
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

