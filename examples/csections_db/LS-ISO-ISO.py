# Conservative Lucas-Saelee (isotropic).
#
# [J Lucas and H T Saelee.
# A comparison of a Monte Carlo simulation and the Boltzmann solution for
# electron swarm motion in gases.
# doi: 10.1088/0022-3727/8/6/007, cite: lucas1975]

[
	{"TYPE":"BACKGROUND", "KEY":"LS-ISO-ISO",
	 "MASSRATE": 1e-3},

	{"TYPE":"ELASTIC",
	 "CSEC":  lambda en, th: 4e-20/sqrt(en),
	},
	{"TYPE":"EXCITATION", "THRESHOLD":15.6,
	 "CSEC":  lambda en, th: 1e-21*(en-th),
	},
]
