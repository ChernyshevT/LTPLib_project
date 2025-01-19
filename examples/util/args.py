#!/usr/bin/env python3

import json, argparse, os

def fix(obj):
	if isinstance(obj, str) and len(obj) and obj[0]=="@":
		return eval(obj[1:])
	
	elif isinstance(obj, dict):
		return {key:fix(obj[key]) for key in obj}

	elif isinstance(obj, list):
		return [fix(val) for val in obj]
	
	else:
		return obj

def is_valid_directory(path):
	if not os.path.isdir(path):
		raise argparse.ArgumentTypeError(f"\"{path}\" is not a valid directory")
	return path

################################################################################
def parse_args(args, cfg):

	if isinstance(cfg, str):
		with open(cfg, "r") as f:
			params = {}; exec(f.read(), {"argparse": argparse}, params)
			entries = params["args"]
	else:
		entries = dict(cfg)
	
	parser = argparse.ArgumentParser()
	for keys, opts in [(arg.split(), entries[arg]) for arg in entries]:
		parser.add_argument(*keys, **opts)

	return parser.parse_args(args[1:])

################################################################################
# Specify the entities to export
__all__ = ["parse_args","argparse"]
