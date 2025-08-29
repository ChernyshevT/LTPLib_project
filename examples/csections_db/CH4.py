# updated 2025-08-25
# database for e+CH4 interactions
[
	{"TYPE":"BACKGROUND", "KEY":"CH4",
	 "MASSRATE":3.420074282530393e-05},
	
	##############################################################################
	# A critical evaluation of low-energy electron impact cross sections for
	# plasma processing modeling. II: Cl4, SiH4, and CH4
	# doi: 10.1007/BF01447255, cite: morgan1992
	# Cross Sections for Electron Collisions with Methane
	# doi: 10.1063/1.4918630,  cite: song2015
	{"TYPE":"ELASTIC",
	 "REF":r"\cite{morgan1992,song2015}",
	 "CSEC":(f"{fpath}/CH4/ELASTIC.txt",
	 dict(search="DATABASE:         Morgan (Kinema Research  Software)")),
	 "MTCS":(f"{fpath}/CH4/ELASTIC.txt",
	 dict(search="DATABASE:         Community database")),
	},

	{"TYPE":"ROTATIONAL",   "THRESHOLD":0.0078,
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="0.0078 / threshold energy", exterp=0)),
	},
	{"TYPE":"ROTATIONAL",   "THRESHOLD":0.0130,
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="0.013 / threshold energy", exterp=0)),
	},
	 
	{"TYPE":"VIBRATIONAL",  "THRESHOLD":0.160,
	 "COMMENT": "V24",
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt", 
	  dict(search="CH4 -> CH4(V24)(0.162eV)")),
	},
	{"TYPE":"VIBRATIONAL",  "THRESHOLD":0.362,
	 "COMMENT": "V13",
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="CH4 -> CH4(V13)(0.362eV)")),
	},

	###################################################################
	# Set of the Electron Collision Cross Sections for Methane Molecule
	# doi: 10.1109/TPS.2018.2885610, cite: gadoum2019
	{"TYPE":"DISSOCIATION", "THRESHOLD":8.8,
	 # this cross-section seems overeestimated in high-energy region!
	 "COMMENT":"CH3 + H", "REF":r"\cite{gadoum2019}",
	 "CSEC": lambda en, th, a=[4.9441,0.3863,1.4973,1.2794]: 
	  1e-20*(a[0]*(th/en)**a[1] * (1 - (th/en)**a[2])**a[3]).real
	}, 
	{"TYPE":"DISSOCIATION", "THRESHOLD":9.4,
	 "COMMENT":"CH2 + H2", "REF":r"\cite{gadoum2019}",
	 "CSEC": lambda en, th, a=[1.2903,0.5079,1.3305,1.2262]: \
	  1e-20*(a[0]*(th/en)**a[1] * (1 - (th/en)**a[2])**a[3]).real
	},
	{"TYPE":"DISSOCIATION", "THRESHOLD":12.5,
	 "COMMENT":"CH + H2 + H", "REF":r"\cite{gadoum2019}",
	 "CSEC": lambda en, th, a=[0.7185,0.6362,1.5057,1.2796]: \
	  1e-20*(a[0]*(th/en)**a[1] * (1 - (th/en)**a[2])**a[3]).real
	},
	{"TYPE":"DISSOCIATION", "THRESHOLD":14.0,
	 "COMMENT":"C + 2H2", "REF":r"\cite{gadoum2019}\footnote{Something is wrong with analytical approximation, data-points were used instead.}",
	# "CSEC": lambda en, th=14.0, a=[0.5681,0.8807,4.0006,1.1164]: \
	#  1e-20*(a[0]*(th/en)**a[1] * (1 - (th/en)**a[2])**a[3]).real
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="CH4 -> C+H2+H2(14eV-dissoc)"))
	},

	##############################################################################
	{"TYPE":"ATTACHMENT",   "THRESHOLD":6.0,
	 "PRODUCTS":"CH3 + H-",
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="CH4 -> CH3+H^-", exterp=0)),
	},
	{"TYPE":"ATTACHMENT",   "THRESHOLD":6.0,
	 "PRODUCTS":"H2 + CH2-",
	 "CSEC":(f"{fpath}/CH4/INELASTIC.txt",
	  dict(search="CH4 -> H2+CH2^-", exterp=0)),
	},

	#####################################################
	# Cross Sections for Electron Collisions with Methane
	# doi:  10.1063/1.4918630, cite: song201
	# eq. (9), table. 15
	{"TYPE":"IONIZATION",   "THRESHOLD":12.63, "OPBPARAM":7.3,
	 "PRODUCTS":"e + CH4+", "REF":r"\cite{song2015}",
	 "CSEC": lambda en, th, a=[1.8034,-1.4809,-3.8281,17.892,-30.666,16.38]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":12.63, "OPBPARAM":7.3,
	 "PRODUCTS":"e + CH3+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[1.5636,-1.3767,-1.7262,11.6948,-23.1158,13.6104]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":16.20, "OPBPARAM":7.3,
	 "PRODUCTS":"e + CH2+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[0.2133,-0.2194,-0.1853,0.8266,-0.1393,0.0044]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":22.20, "OPBPARAM":7.3,
	 "PRODUCTS":"e + CH+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[-0.1661,0.1893,-0.3884,4.0615,-5.8045,3.2324]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":22.00, "OPBPARAM":7.3,
	 "PRODUCTS":"e + C+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[-0.1234,0.0362,0.5527,-0.6303,0.5648,-0.1526]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":22.30, "OPBPARAM":7.3,
	 "PRODUCTS":"e + H2+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[-0.0058,0.0088,-0.077,0.2865,0.1644,-0.2252]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
	{"TYPE":"IONIZATION",   "THRESHOLD":21.10, "OPBPARAM":7.3,
	 "PRODUCTS":"e + H2+", "REF":r"\cite{song2015}",
	 "CSEC":lambda en, th, a=[-0.4317,0.3519,1.4791,-5.5021,11.5604,-4.6928]: \
	 1e-17/th/en * (a[0]*log(en/th) \
	 + sum(a*(1-th/en)**j for j,a in enumerate(a[1:], 1)))
	},
]
