#!/usr/bin/env python3
import os, io, json, zipfile, json
import numpy as np

from types import SimpleNamespace as namespace
from enum import Enum

from .loggers import *

class ENTRYT(Enum):
	CACHE = 5,
	FUNC  = 4,
	JSON  = 3,
	NUMPY = 2,
	TEXT  = 1,

################################################################################
def to_namspace(obj):
	if   isinstance(obj, dict):
		return namespace(**{k: to_namspace(obj[k]) for k in obj})
	elif isinstance(obj, list):
		return [to_namspace(item) for item in obj]
	elif isinstance(obj, tuple):
		return tuple(to_namspace(item) for item in obj)
	elif isinstance(obj, set):
		return {to_namspace(item) for item in obj}
	else:
		return obj

################################################################################
class npEncoder(json.JSONEncoder):
	def default(self, obj):
		if isinstance(obj, np.integer):
			return int(obj)
		if isinstance(obj, np.floating):
			return float(obj)
		if isinstance(obj, np.ndarray):
			return obj.tolist()
		if isinstance(obj, namespace):
			return vars(obj)
		if isinstance(obj, (set, tuple)):
			return list(obj)
		return json.JSONEncoder.default(self, obj)

################################################################################

logger = get_logger()

class frame_cls:
	__slots__ = ("__zipf", "__list", "__funcs", "__cache")

	def __init__(self, fname: str, **kwargs):
		self.__zipf  = zipfile.ZipFile(fname, "r")
		self.__list  = [os.path.splitext(f)[0] for f in self.__zipf.namelist()]
		self.__funcs = {}
		self.__cache = {}
		logger.debug(f"open {fname}")
	
	def add_funcs (self, **kwargs):
		self.__funcs.update({**kwargs})
		return self
	
	def __dir__(self):
		return [*self.__list, *self.__funcs]

	def __getitem__(self, key: str):
		match (self.__contains__(key)):
			
			case ENTRYT.CACHE:
				pass
			
			case ENTRYT.FUNC:
				self.__cache[key] = self.__funcs[key](self)
				if self.__cache[key] is None:
					del self.__cache[key]
					del self.__funcs[key]
			
			case ENTRYT.JSON:
				self.__cache[key] = to_namspace(json.loads \
				(io.BytesIO(self.__zipf.read(f"{key}.json")).read().decode("utf-8")))
			
			case ENTRYT.NUMPY:
				self.__cache[key] = np.load \
				(io.BytesIO(self.__zipf.read(f"{key}.npy")))
			
			case ENTRYT.TEXT:
				self.__cache[key] = self.__zipf.read(f"{key}.txt").decode("utf-8")
			
			case _:
				return None
		
		return self.__cache[key]

	def __getattr__(self, key: str):
		return self[key]

	def __contains__(self, key: str):
		if key in self.__cache:
			return ENTRYT.CACHE
		if key in self.__funcs:
			return ENTRYT.FUNC
		if f"{key}.json" in self.__zipf.namelist():
			return ENTRYT.JSON
		if f"{key}.npy"  in self.__zipf.namelist():
			return ENTRYT.NUMPY
		if f"{key}.txt"  in self.__zipf.namelist():
			return ENTRYT.TEXT
		return False

def load_frame(fname: str) -> frame_cls:
	return frame_cls(fname)

################################################################################

def save_frame(fname: str, mode: str = "w", **kwargs):
	fname = os.path.abspath(os.path.expanduser(fname))
	msg   = f"saving \"{fname}\".."
	try:
		
		if mode != "w" and mode != "a":
			raise ValueError(f"mode = {mode}")
		
		os.makedirs(os.path.dirname(fname), exist_ok=True)
		#
		with zipfile.ZipFile(fname, mode, compression=zipfile.ZIP_STORED) as zipf:
			#
			def save_entry(k, arg):
				
				if   isinstance(arg, np.ndarray):
					dump = io.BytesIO(); np.save(dump, arg)
					zipf.writestr(f"{k}.npy"
					, data=dump.getbuffer().tobytes())
				elif isinstance(arg, str):
					zipf.writestr(f"{k}.txt"
					, data=arg.encode("utf-8"))
				else:
					zipf.writestr(f"{k}.json"
					, data=json.dumps(arg, cls=npEncoder, indent="\t").encode("utf-8"))
			#
			for k,arg in kwargs.items():
				save_entry(k, arg)
			nsize = sum([zinfo.file_size for zinfo in zipf.filelist])
			#
			szstr = " KMGT"
			while nsize > 512:
				nsize = nsize/1024;
				szstr = szstr[1:]
			logger.info(f"{msg} ok ({nsize:.2f}{szstr[0]}B)")
	
	except Exception as e:
		logger.error(f"{msg} fail: \"{e}\"")
		raise e

################################################################################
# Specify the entities to export
__all__ = ["load_frame", "save_frame", "namespace"]
