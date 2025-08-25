################################################################################
# Reid's ramp model gas: anisotropic scattering (screened Coulomb).
# From: Benchmark calculations for anisotropic scattering in kinetic models
# for low temperature plasma
# doi:10.1088/1361-6463/ad3477 , cite: flynn2024
[
	{"TYPE":"BACKGROUND", "KEY":"RR-SC-SC",
	 "MASSRATE": 1.371450e-04}, # me/(4AMU)

	{"TYPE":"ELASTIC",
	 "MTCS":  lambda en, th: 6e-20,
	 "DCSFN": lambda en, es=27.21: (4*en/es)/(1+4*en/es),
	 "REF":r"\cite{flynn2024}",
	},
	{"TYPE":"EXCITATION", "THRESHOLD":0.2,
	 "MTCS":  lambda en, th: 10*(en-th)*1e-20,
	 "DCSFN": lambda en, es=27.21: (4*en/es)/(1+4*en/es),
	 "REF":r"\cite{flynn2024}",
	},
]
