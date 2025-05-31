################################################################################
# Reid's ramp model gas: anisotropic scattering (screened Coulomb).
# From: Benchmark calculations for anisotropic scattering in kinetic models
# for low temperature plasma
# doi:10.1088/1361-6463/ad3477 , cite: flynn2024
[
	{"TYPE":"PARTICLE", "KEY":"e",
	 "ENCFFT": 2.842815e-16}, # (cm/s)² -> eV (CGS)
	{"TYPE":"BACKGROUND", "KEY":"RR-SC-SC",
	 "MASSRATE":9.109383e-28/(4*1.660538921e-24)},

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
