# database for e+MAXWELL_GAS interaction (benchmark)
[
	# ~ {"TYPE":"PARTICLE", "KEY":"e",
	 # ~ "ENCFFT":299.792458*0.5*9.109383e-28/4.803204e-10}, # speed² -> eV
	{"TYPE":"BACKGROUND", "KEY":"MAXWELL",
	 "MASSRATE":5e-1},

	{"TYPE":"ELASTIC", # 1e-8 cm^3/s
	 "CSEC":  lambda en,th: 1.686056314280897e-20/sqrt(en)},
]
