#!/usr/bin/env python3
import os, io, zipfile, json, mmap, struct
import numpy as np

from types import SimpleNamespace
from .loggers import *

################################################################################
def to_namspace(obj):
	if   isinstance(obj, dict):
		return SimpleNamespace(**{k: to_namspace(obj[k]) for k in obj})
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
		if isinstance(obj, SimpleNamespace):
			return vars(obj)
		if isinstance(obj, (set, tuple)):
			return list(obj)
		return json.JSONEncoder.default(self, obj)

################################################################################

logger = get_logger()

class frame_cls:
	__slots__ = ("_zipf", "_list", "_noex", "files")

	def __init__(self, fname: str, noexcept: bool):
		self._zipf = zipfile.ZipFile(fname, "r")
		self._list = [os.path.splitext(f)[0] for f in self._zipf.namelist()]
		self._noex = noexcept
		self.files = self._zipf.namelist()
		logger.info(f"open {fname}")
	
	def __dir__(self):
		return self._list

	def __getitem__(self, key: str):
		import io, json
		###############################
		if f"{key}.json" in self.files:
			return to_namspace(json.loads\
			(io.BytesIO(self._zipf.read(f"{key}.json")).read().decode("utf-8")))
		###############################
		if f"{key}.npy"  in self.files:
			return np.load\
			(io.BytesIO(self._zipf.read(f"{key}.npy")))
		###############################
		if f"{key}.txt"  in self.files:
			return self._zipf.read(f"{key}.txt").decode("utf-8")
		###############################
		if self._noex:
			return None
		else:
			raise ValueError(f"invalid key: \"{key}\"")
		
	def __getattr__(self, key: str):
		return self[key]

	def __contains__(self, key: str):
		if f"{key}.json" in self.files:
			return 3
		if f"{key}.npy"  in self.files:
			return 2
		if f"{key}.txt"  in self.files:
			return 1
		return 0
	
	def __iter__(self):
		for key in self._list:
			yield key, self[key]
	
	def __len__(self):
		return len(self._list)

def load_frame(fname: str, noexcept: bool=True) -> frame_cls:
	return frame_cls(fname, noexcept)

################################################################################

def save_frame(fname: str, mode:str ="w", **kwargs):
	fname = os.path.abspath(os.path.expanduser(fname))
	msg   = f"save \"{fname}\".."
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
					zipf.writestr(f"{k}.npy",
						data=dump.getbuffer().tobytes())
				elif isinstance(arg, str):
					zipf.writestr(f"{k}.txt",
						data=arg.encode("utf-8"))
				else:
					zipf.writestr(f"{k}.json",
						data=json.dumps(arg, cls=npEncoder, indent="\t").encode("utf-8"))
			#
			for k,arg in kwargs.items():
				save_entry(k, arg)
			nsize = sum([zinfo.file_size for zinfo in zipf.filelist])
			
			szstr = " KMGT"
			while nsize > 512:
				nsize /= 1024; szstr = szstr[1:]
				
			logger.info(f"{msg} ok ({nsize:.2f}{szstr[0]}B)")
	except Exception as e:
		logger.error(f"{msg} fail: \"{e}\"")
		raise e

# Specify the entities to export
__all__ = ["load_frame", "save_frame"]
