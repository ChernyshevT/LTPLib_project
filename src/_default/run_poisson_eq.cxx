#include "typedefs.hxx"
#include "api_backend.hxx"
#include "common/poisson_eq.hxx"

#include <tgmath.h>
#include <math.h>

/*******************************************************************************
 * An implementation for SOR+GS method, see [Mittal(2014) S Mittal.
 * A study of successive over-relaxation method parallelisation over modern
 * HPC languages. IJHPCN. 7(4):292, 2014. doi: 10.1504/ijhpcn.2014.062731].
 ******************************************************************************/
template<u8 nd>
f32 run_SOR_iter (poisson_eq_t<nd> & eq, f32 w) {
	
	f32 *vcache = eq.vdata + eq.offst[0];
	
	/* cache old values */
	#pragma omp parallel for
	for (u64 uid=0; uid<eq.offst[0]; ++uid) {
		vcache[uid] = eq.vdata[uid];
	}
	
	/* loop over red/black-units */
	for (u8 nseq : {0,1}) {
		#pragma omp parallel for
		for (u64 uid=0; uid<eq.offst[0]; ++uid) {
			u32 pos[nd];
			u32 sum{0};
			u64 remain{uid};
			for (u8 i{0u}; i<nd; ++i) {
				pos[i] = (u32)(remain / eq.offst[i+1]);
				remain = remain % eq.offst[i+1];
				sum += pos[i];
			}
			if ( (u8)(sum%2) == nseq) {
				vcache[uid] = eq.get_vnew(pos, vcache);
			}
		}
	}
	
	/* perform SOR-step */
	f32 verr{0.0f}, vold, vnew;
	#pragma omp parallel for reduction(max:verr) private(vold, vnew)
	for (u64 uid=0; uid<eq.offst[0]; ++uid) {
		vold = eq.vdata[uid];
		vnew = vcache[uid];
		vnew = w*vnew + (1.0f-w)*vold;
		if (isfinite(vnew)) {
			verr = std::max(fabsf(vnew - vold), verr);
		}
		eq.vdata[uid] = vnew;
	}
	
	return verr;
}

#include "run_poisson_eq_fns.cxx"




