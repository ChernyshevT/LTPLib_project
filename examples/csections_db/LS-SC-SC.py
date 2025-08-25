# Conservative Lucas-Saelee + anisotropic scattering (screened Coulomb).
#
# [J Lucas and H T Saelee.
# A comparison of a Monte Carlo simulation and the Boltzmann solution for
# electron swarm motion in gases.
# doi: 10.1088/0022-3727/8/6/007, cite: lucas1975]
#
# [Benchmark calculations for anisotropic scattering in kinetic models
# for low temperature plasma
# doi:10.1088/1361-6463/ad3477, cite: flynn2024]
[
	{"TYPE":"BACKGROUND", "KEY":"LS-SC-SC",
	 "MASSRATE": 1e-3},

	{"TYPE":"ELASTIC",
	 "MTCS":  lambda en, th: 4e-20/sqrt(en),
	 "DCSFN": lambda en, es=27.21: (4*en/es)/(1+4*en/es),
	},
	{"TYPE":"EXCITATION", "THRESHOLD":15.6,
	 "MTCS":  lambda en, th: 1e-21*(en-th),
	 "DCSFN": lambda en, es=27.21: (4*en/es)/(1+4*en/es),
	},
]
