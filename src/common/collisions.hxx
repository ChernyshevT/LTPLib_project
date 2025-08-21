#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/
#pragma once
#include "typedefs.hxx"
#include <tgmath.h>
#include <math.h>

#include "common/rng.hxx"
#include "common/scatter.hxx"

enum cltype : u16 {
	ERROR_ENLIMIT,
	ERROR_PTMAXPROBABILITY,
	CONSERVATIVE,
	IONIZATIONRUN,
	IONIZATIONEND,
	ATTACHMENT,
	NONE,
};

inline const char *CLTYPE_DESCR[] = {
	"ERROR_ENLIMIT",
	"ERROR_PTMAXPROBABILITY",
	"CONSERVATIVE",
	"IONIZATIONRUN",
	"IONIZATIONEND",
	"ATTACHMENT",
	"NONE",
};

struct collision_t {

	rng_t &  rng;
	
	cltype   type : 4;
	u16 chnl : 12;
	
	u8  nsee;
	
	u8  bgid;
	
	struct {
		u16 vtemp_id = 0;
		u16 uflux_id = 0;
		u16 mrate_id = 0;
	} cfg;
	
	f32 // various parameters
		mrate, param;
	f32 // collision energies
		enel, enth;
	f32 // ionization energies
		ensys, enpri, ensec; 
	f32 // speed \& velocity
		vabs, vdir[3], wdir[3], speed;
	f32 // collision parameters
		sina, cosa, sinb, cosb;

	inline collision_t (rng_t& rng_in) : rng(rng_in) {
		type  = cltype::NONE;
		chnl  = 0;
		mrate = 0.0f;
		param = 0.0f;
	};
	
	/* set background stuff *****************************************************/

	inline f32 do_energy (f32 v[], f32 ecff) {
		/*
		switch (bgflag) {
			case : // velocity
			case : // temperature
			default:
		}
		*/
		
		vabs = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
		enel = vabs*ecff;
		vabs = sqrtf(vabs);
		vdir[0] = v[0]/vabs;
		vdir[1] = v[1]/vabs;
		vdir[2] = v[2]/vabs;
		return enel;
	}
	
	/* conservative collision ***************************************************/
	inline void do_conservative (f32 v[]) {
		/* energy-impulse balance:
		 * [METHES: A Monte Carlo collision code for the simulation of electron
		 * transport in low temperature plasma. doi: 10.1016/j.cpc.2016.02.022]
		 * */
		f32 beta, rval, frac;
		do { /* loop to reject invalid values! */
			rval = rng.uniform(0.0f, 1.0f);
			// [TODO: CHECK cosa!]
			cosa = 1.0f - 2*rval*(1.0f - param)/(1.0f + param - 2.0f*rval*param);
			sina = sqrtf(1.0f-cosa*cosa);
			frac = sqrtf(ensys/enel - mrate*(1.0f-cosa));
		} while (!isfinite(sina) or !isfinite(frac));
		speed = vabs*frac;
		
		beta = rng.uniform(0.0f, 2*M_PI);
		cosb = cosf(beta);
		sinb = sinf(beta);

		scatter(wdir, vdir, sina, cosa, sinb, cosb);
		for (auto j{0}; j<3; ++j) {
			v[j] = wdir[j]*speed;
		}
		type = cltype::NONE;
	}
	
	/* ionization collision *****************************************************/
	inline void do_ionization_run (f32 v[]) {
		/* energy-impulse balance using OPB-approximation */
		f32 beta, rval, factor{atanf(0.5f*ensys/param)};
		do { /* loop to reject invalid values! */
			rval  = rng.uniform(0.0f, 1.0f);
			ensec = param*tanf(rval*factor);
			enpri = ensys-ensec;
			cosa  = sqrtf(ensec/ensys);
			sina  = sqrtf(1.0f - cosa*cosa);
		} while (!isfinite(sina) or enpri <= 0.0f);
		
		// secondary particle
		beta = rng.uniform(0.0f, 2*M_PI);
		cosb = cosf(beta);
		sinb = sinf(beta);
		scatter(wdir, vdir, sina, cosa,  sinb,  cosb);
		speed = vabs*sqrtf(ensec/enel);
		for (auto i{0}; i<3; ++i) {
			v[i] = wdir[i]*speed;
		}
		// primary particle
		cosa = sqrtf(enpri/ensys);
		sina = sqrtf(1.0f - cosa*cosa);
		scatter(wdir, vdir, sina, cosa, -sinb, -cosb);
		if (--nsee) {
			/* keep energy for the next iteration if (nsee >= 1) */
			ensys = enpri;
			for (auto i{0}; i<3; ++i) {
				vdir[i] = wdir[i];
			}
		} else {
			type = cltype::IONIZATIONEND;
		}
	}
	
	inline void do_ionization_end (f32 v[]) {
		speed = vabs*sqrtf(enpri/enel);
		for (auto i{0}; i<3; ++i) {
			v[i] = wdir[i]*speed;
		}
		type = cltype::NONE;
	}
};
