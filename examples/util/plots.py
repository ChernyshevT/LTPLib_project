import os, sys
import numpy as np

TeX_preamble =\
r"""
\usepackage[utf8]{inputenc}
\usepackage[english]{babel}
\usepackage{amsmath}
\usepackage{amssymb}
\usepackage{gensymb}
%\usepackage{chemformula}
\usepackage[separate-uncertainty=true]{siunitx}
	\DeclareSIUnit \gauss{G}
	\DeclareSIUnit \atmosphere{atm}
	\DeclareSIUnit \townsend{Td}
\usepackage{physics}
"""
import matplotlib as mp
mp.rc("font",       size=10, family="serif", serif=["Computer Modern Roman"])
mp.rc("lines",      linewidth=0.5)
mp.rc("text",       usetex=True)
mp.rc("text.latex", **{"preamble":TeX_preamble})
import matplotlib.pyplot as plt

################################################################################
def mk_subplots(ws, hs, name="", **kws):
	wlf,wax,wrt = ws[:3]
	wsp = 0 if len(ws)<4 else ws[3]
	hdn,hax,htp = hs[:3]
	hsp = 0 if len(hs)<4 else hs[3]

	w = wlf + wax*kws.get("ncols",1) + wrt + wsp
	h = hdn + hax*kws.get("nrows",1) + htp + hsp
	fig, axs = plt.subplots(figsize=(w/2.54,h/2.54), **kws)
	
	w_opts = dict(left=wlf/w, right=(w-wrt)/w, wspace=wsp/wax)
	h_opts = dict(bottom=hdn/h, top=(h-htp)/h, hspace=hsp/hax)
	fig.subplots_adjust(**w_opts, **h_opts)
	
	if name:
		fig.name = name
	
	return fig, axs.flatten() if isinstance(axs, np.ndarray) else axs

################################################################################
def make_legend(ax, lines, labels, **kwargs):
	conf = {
		"bbox_to_anchor":(1,0,1,1),
		"fontsize":8,
		"borderaxespad":0,
		"columnspacing":1,
		"labelspacing" :0.125,
		"frameon":0,
		"handlelength":.75,
		"loc":"center left",
		"ncol":1,
		**kwargs,
	}
	return ax.legend(lines[::-1], labels[::-1], **conf)

################################################################################
def save_fig(fig, **kwargs):
	
	import logging; logger = logging.getLogger();
	
	if name := kwargs.get("name"):
		fig.name = name
	
	if fpath := kwargs.get("fpath"):
		fname = f"{fpath}/{fig.name}.{kwargs.get('fmt', 'pdf')}"
	else:
		fname = f"{fig.name}.{kwargs.get('fmt', 'pdf')}"
		
	fname = os.path.abspath(os.path.expanduser(fname))
	
	msg   = f"save \"{fname}\".."
	try:
		os.makedirs(os.path.dirname(fname), exist_ok=True)
		fig.savefig(fname, dpi=kwargs.get("dpi", 300))
		logger.info(f"{msg} ok")
	except Exception as e:
		logger.error(f"{msg} fail: \"{e}\"")
		raise e

################################################################################
def show_streams(ax, step, order):
	pass

	
################################################################################
def show_field(ax, *fs, idx=None, bins=None, **kw):

	# ~ print(ax, [*fs], idx, bins, {**kw})
	
	from matplotlib.colors import LogNorm
	opts = {
		"origin": "lower",
		"interpolation":"antialiased",
		"alpha":1,
		"cmap":"jet",
		"aspect":"auto",
	}
	if "scale" in kw and kw["scale"]=="log":
		opts["norm"]=LogNorm(kw["vmin"], kw["vmax"])
		
		# ~ print(kw.keys())
		del kw["scale"]
		del kw["vmin"]
		del kw["vmax"]
	opts.update(kw);
	# ~ if "norm" in opts:
		# ~ del opts["vmin"]
		# ~ del opts["vmax"]
		# ~ del opts["scale"]
	
	ims = []
	xmn = ymn = +np.inf
	xmx = ymx = -np.inf
	vmn = vmx = 0
	for f, ex in fs:
		data = f[...,idx].real.T if idx is not None else f.real.T
		if ex[0]<xmn: xmn = ex[0]
		if ex[1]>xmx: xmx = ex[1]
		if ex[2]<ymn: ymn = ex[2]
		if ex[3]>ymx: ymx = ex[3]
		vmx1 = np.max(data)
		vmn1 = np.min(data)
		if vmn1<vmn: vmn = vmn1
		if vmx1>vmx: vmx = vmx1
	for f, ex in fs:
		data = f[...,idx].real.T if (idx is not None) else f.real.T
		ims.append(ax.imshow(data, extent=ex, **opts))

	if not bins:
		ax.set_xlim(xmn, xmx); ax.set_ylim(ymn, ymx)
	else:
		ax.set_xlim(bins[0][0], bins[0][-1])
		ax.set_ylim(bins[1][0], bins[1][-1])
		ax.set_xticks(bins[0], minor=0)
		ax.set_yticks(bins[1], minor=0)
		ax.grid(1, which='major', c="k", ls="--", lw=0.25)


	return tuple(ims) if len(ims)>1 else ims[0]
################################################################################

def mk_rtax(ax, **kwargs):
	from mpl_toolkits.axes_grid1 import make_axes_locatable
	fig = ax.get_figure()
	width, height          = fig.get_figwidth(), fig.get_figheight()
	xpos, ypos, xlen, ylen = make_axes_locatable(ax).get_position()
	return fig.add_axes([xpos+xlen+0.05/2.54/width, ypos, 0.2/2.54/width,ylen], **kwargs)

def mk_cax(ax, **kwargs):
	from mpl_toolkits.axes_grid1 import make_axes_locatable
	# ~ print(im)
	
	# ~ fig = ax.get_figure()
	# ~ xpos, ypos, xlen, ylen =\
		# ~ make_axes_locatable(ax).get_position()
		
	# ~ dx = 0.25/2.54 /fig.get_figwidth()
	# ~ dy = 0.125/2.54/fig.get_figheight()
	# ~ y_ = 1.5/2.54  /fig.get_figheight()*2/3
	# ~ #xy = [xpos+xlen/2+dx,ypos+ylen+y_,xlen/2-dx*2,dy]
	# ~ xy = [xpos+xlen/2+dx,ypos+ylen+y_, xlen/2-dx*2, dy]
	# ~ return fig.add_axes(xy, clip_box={"fc":"w","pad":0.5,"lw":0.5}, frame_on=1)
	
	fig = ax.get_figure()
	width, height          = fig.get_figwidth(), fig.get_figheight()
	xpos, ypos, xlen, ylen = make_axes_locatable(ax).get_position()
	return fig.add_axes([xpos+xlen+0.05/2.54/width, ypos, 0.2/2.54/width,ylen], **kwargs)

def show_cbar(im, ax, **kwargs):
	


	cbar = plt.colorbar(im, cax=mk_cax(ax), **kwargs)
	cbar.ax.tick_params(labelsize=4)

	# ~ ax.tick_params(top=True, labeltop=True, bottom=False, labelbottom=False)
	
	return cbar

################################################################################
from matplotlib.font_manager import FontProperties
from matplotlib.ticker import Locator
import matplotlib.ticker
class MinorSymLogLocator(Locator):
	"""
	Dynamically find minor tick positions based on the positions of
	major ticks for a symlog scaling.
	"""
	def __init__(self, linthresh=1, nints=10):
		"""
		Ticks will be placed between the major ticks.
		The placement is linear for x between -linthresh and linthresh,
		otherwise its logarithmically. nints gives the number of
		intervals that will be bounded by the minor ticks.
		"""
		self.linthresh = linthresh
		self.nintervals = nints

	def __call__(self):
		# Return the locations of the ticks
		majorlocs = self.axis.get_majorticklocs()

		if len(majorlocs) == 1:
			return self.raise_if_exceeds(np.array([]))

		# add temporary major tick locs at either end of the current range
		# to fill in minor tick gaps
		dmlower = majorlocs[1] - majorlocs[0]    # major tick difference at lower end
		dmupper = majorlocs[-1] - majorlocs[-2]  # major tick difference at upper end

		# add temporary major tick location at the lower end
		if majorlocs[0] != 0. and ((majorlocs[0] != self.linthresh and dmlower > self.linthresh) or (dmlower == self.linthresh and majorlocs[0] < 0)):
			majorlocs = np.insert(majorlocs, 0, majorlocs[0]*10.)
		else:
			majorlocs = np.insert(majorlocs, 0, majorlocs[0]-self.linthresh)

		# add temporary major tick location at the upper end
		if majorlocs[-1] != 0. and ((np.abs(majorlocs[-1]) != self.linthresh and dmupper > self.linthresh) or (dmupper == self.linthresh and majorlocs[-1] > 0)):
			majorlocs = np.append(majorlocs, majorlocs[-1]*10.)
		else:
			majorlocs = np.append(majorlocs, majorlocs[-1]+self.linthresh)

		# iterate through minor locs
		minorlocs = []

		# handle the lowest part
		for i in range(1, len(majorlocs)):
			majorstep = majorlocs[i] - majorlocs[i-1]
			if abs(majorlocs[i-1] + majorstep/2) < self.linthresh:
				ndivs = self.nintervals
			else:
				ndivs = self.nintervals - 1.

			minorstep = majorstep / ndivs
			locs = np.arange(majorlocs[i-1], majorlocs[i], minorstep)[1:]
			minorlocs.extend(locs)

		return self.raise_if_exceeds(np.array(minorlocs))

	def tick_values(self, vmin, vmax):
		raise NotImplementedError\
		('Cannot get tick locations for a ' '%s type.' % type(self))

################################################################################
from matplotlib.colors import LogNorm

# Specify the entities to export
__all__ = ["mk_subplots", "save_fig", "show_field", "show_cbar","mk_cax","mk_rtax", "plt", "LogNorm", "mp", "MinorSymLogLocator","make_legend"]
