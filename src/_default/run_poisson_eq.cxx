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
	
	f32 verr{0.0f}, vold, vnew;
	
	/* loop over red/black-units & perform SOR-step */
	for (u8 nseq : {0,1}) {
		
		#pragma omp parallel for private(vold, vnew) reduction(max:verr)
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
				vold = eq.vdata[uid];
				vnew = eq.get_vnew(pos, eq.vdata);
				vnew = w*vnew + (1.0f-w)*vold;
				if (isfinite(vnew)) {
					verr = std::max(fabsf(vnew - vold), verr);
				}
				eq.vdata[uid] = vnew;
			}
		}
	}
	
	return verr;
}

#include "run_poisson_eq_fns.cxx"




