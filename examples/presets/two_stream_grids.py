{
 "hires2d" : {"nd": 2,
  "step"  : [0.00625 for _ in "xy"],
  "axes"  : [[*range(0,128+1,16)] for _ in "xy"],
  "flags" : "LOOPX|LOOPY",
 },
 "lowres3d": {"nd": 3,
  "step"  : [0.025 for _ in "xyz"],
  "axes"  : [[*range(0,32+1,8)] for _ in "xyz"],
  "flags" : "LOOPX|LOOPY|LOOPZ",
 },
}
