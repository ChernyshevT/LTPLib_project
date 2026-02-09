#!/usr/bin/env python3

import numpy as np
import pandas as pd
import os
import shutil

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

def draw_eVDF(fname, ax, **kwargs):
	
	vmx = kwargs.get("vmx", 1e9)
	
	xs,ys = np.linspace(-vmx, +vmx, 101), np.linspace(-vmx, +vmx, 101)
	dist = distro_h(xs, ys)
	
	pdata = load_frame(fname)
	parts = pdata.data[pdata.index[0]: pdata.index[1]]
	np.random.shuffle(parts)
	
	vxs,vys,vzs = parts.T[-3:]
	dist.add(vxs, vys).normalize()
	n = min(kwargs.get("n", pdata.index[1]), pdata.index[1])
	
	cols = dist.get(vxs[:n], vys[:n])
	
	ux,uy = np.mean(vxs), np.mean(vys)

	ptcfg = {
	 "cmap"       : "jet",
	 "rasterized" : 1,
	 "marker"     : "s",
	 "s"          : kwargs.get("msize", 16)*(72./ax.figure.dpi)**2,
	 "lw"         : 0,
	 **(dict(norm=kwargs.get("norm")) if "norm" in kwargs else {})
	}
	
	cm = kwargs.get("cmap", "jet")
	
	ax.imshow(dist.hist.T*1e3, extent=(-vmx, +vmx, -vmx, +vmx), cmap=cm, origin="lower")
	# ~ im = ax.scatter(vxs[:n], vys[:n], c="k", **ptcfg)
	# ~ ax.plot(vxs[:n], vys[:n],",k")
	# ~ return im

	ax.axhline(0, ls="--", c="w")
	ax.axvline(0, ls="--", c="w")

	ax.plot(ux, uy, "xw")
	if u0 := kwargs.get("u0"):
		ax.axhline(u0, ls="--", c="w")
		alpha = np.linspace(0,2*np.pi,361)
		xx,yy = u0*np.sin(alpha), u0+u0*np.cos(alpha)
		ax.plot(xx,yy,"--w",lw=1)
	

################################################################################

funcs = {
"TIME": lambda f:\
 0.5*np.sum(f.cfg.tindex)*f.cfg.dt * 1e9,

"ENe": lambda f:\
 np.nanmean(f.ptfluid[...,1]/f.ptfluid[..., 0])*2.842815e-16,

"UXe": lambda f:\
 np.nanmean(f.ptfluid[...,2]/f.ptfluid[..., 0]),

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
	
	if os.path.exists(fname:=f"{os.path.abspath(args.fdir)}.dset.zip"):
		if input(f"\"{fname}\" already exists, delete [y|yes]? ") in ["y", "yes"]:
			shutil.rmtree(fname)
		else:
			exit(0)
	
	##############################################################################
	stats, n = [], 0
	while os.path.exists(fname:=f"{args.fdir}/frame{n+1:06d}.zip"):
		print(f"\rread \"{fname}\"", end="")
		frame = load_frame(fname).add_funcs(**funcs)
		stats.append({k:frame[k] for k in funcs})
		n = n+1
	print("\rdone")
	stats = pd.DataFrame(stats)
	
	fields = ["ENe", "UDRIFTe", "EVENTS/PT"]

	line = f"| #FRAME| ERRMAX|"\
	     + "|".join([f"{key:>10}" for key in fields]) + "|"
	print(f"{line}\n{'-'*len(line)}")
	
	k_avg = None
	for j, row in stats.iterrows():
		
		errmx = None
		if j >= args.window:
			a, b = j-args.window+1, j+1
			for field in fields:
				err = np.std(stats[field][a:b])/np.mean(stats[field][a:b])
				if errmx is not None:
					errmx = max(abs(err), errmx)
				else:
					errmx = abs(err)

		line = f"| {j+1:>6d}"\
		     + (f"| {errmx*1e2:>5.2f}%|" if errmx else "| ----- |")\
		     +  "|".join([f"{row[key]:>10.3e}" for key in fields]) + "|"
		print(f"{line}")

		if k_avg is None and errmx is not None and errmx < args.precision:
			k_avg = j
			print('-'*len(line))
		
		if k_avg is not None and errmx is not None and errmx >= args.precision:
			raise RuntimeError(f"errmx = {errmx:e} >= {args.precision:e}!")
	
	# endline ##########
	print('-'*len(line))
	
	###################################
	dset = {"cfg": frame.cfg, "avg":{}}
	for key in keys:
		entry = np.stack(stats[k_avg:][key])
		dset["avg"][key] = entry.mean(axis=0), entry.std(axis=0)
	
	#############
	if args.eVDF:
		vmax = float(args.eVDF[0])
		npts = int(args.eVDF[1])
		
		xbins,ybins = [np.linspace(-vmax, +vmax, npts) for _ in range(2)]
		dist = distro_h(xbins, ybins)
		
		pdata, count = None, 0
		for k in range(k_avg, n+1):
			if os.path.exists(fname:=f"{args.fdir}/pdata{k:06d}.zip"):
				pdata  = load_frame(fname)
				vxs,vys,vzs = pdata.data.T[2:]
				dist.add(vxs, vys)
				count += 1
		
		if pdata is None:
			raise RuntimeError("no pdata-files were found!")
		else:
			dset["eVDFxy"] = dist.hist/count
			dset["vmax"]   = vmax
			dset["parts"]  = pdata.data.T[2:]
	
	############################################################
	print(dset.keys())
	save_frame(f"{os.path.abspath(args.fdir)}.dset.zip", **dset)
	
################################################################################

args = {
 "--fdir" : {
  "type": str,
  "required": True
 },
 "--window" : {
  "type": int,
  "default": 10,
  "required": False,
 },
 "--precision": {
  "type": float,
  "default": 0.01,
  "required": False,
 },
 "--eVDF" : {
  "nargs": 2,
  "required": False,
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

