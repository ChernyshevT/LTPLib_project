{
 "hires2d" : {"nd": 2,
  "step"  : [0.00625 for _ in "xy"],
  "axes"  : [[*range(0,192+1,12)] for _ in "xy"],
  "flags" : "LOOPX|LOOPY",
 },
 "lowres3d": {"nd": 3,
  "step"  : [0.025 for _ in "xyz"],
  "axes"  : [[*range(0,48+1,6)] for _ in "xyz"],
  "flags" : "LOOPX|LOOPY|LOOPZ",
 },
}
