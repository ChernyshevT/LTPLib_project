# database for e + MAXWELL gas interaction (benchmark)
[
	{"TYPE":"BACKGROUND", "KEY":"MAXWELL",
	 "MASSRATE":5e-1},

	{"TYPE":"ELASTIC", # 1e-8 cm^3/s
	 "CSEC":  lambda en,th: 1.686056314280897e-20/sqrt(en)},
]
