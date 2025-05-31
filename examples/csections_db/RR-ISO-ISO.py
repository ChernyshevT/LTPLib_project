################################################################################
# Reid's ramp model gas: isotropic scattering.
# From: Multi-term solution of the Boltzmann equation for electron swarms
# in crossed electric and magnetic fields.
# doi: 10.1088/0022-3727/27/9/007, cite: ness1994
[
	{"TYPE":"PARTICLE", "KEY":"e",
	 "ENCFFT": 2.842815e-16}, # (cm/s)² -> eV (CGS)
	{"TYPE":"BACKGROUND", "KEY":"RR-ISO-ISO",
	 "MASSRATE":9.109383e-28/(4*1.660538921e-24)},
	
	{"TYPE":"ELASTIC",
	 "CSEC":  lambda en, th: 6e-20,
	 "REF":"\cite{ness1994}",
	},
	{"TYPE":"EXCITATION", "THRESHOLD":0.2,
	 "CSEC":  lambda en, th: 10*(en-th)*1e-20,
	 "REF":"\cite{ness1994}",
	},
]
