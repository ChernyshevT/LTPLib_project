#include "typedefs.hxx"
#include "api_backend.hxx"
#include "common/poisson_eq.hxx"

#include <tgmath.h>
#include <math.h>

/*******************************************************************************
 * (2^nd * (nd+1))-length bit-mask to loop over the array in a red/black order
 ******************************************************************************/
consteval u32 red_black_seq(u32 nd) {
	u32 seq{0u};

	for (u8 i{0u}; i < (1u<<nd); i++) {
		u32 is_odd = 0;
		for (u8 j{0u}; j < nd; j++) {
			is_odd += (i >> j) & 1u;
		}
		is_odd = is_odd%2;

		u32 shift = (nd+1)*(i/2) + is_odd*(nd+1)*(1u<<(nd-1));
		u32 value = (1u<<nd) | i;

		seq = seq | (value << shift);
	}

	return seq;
}

/*******************************************************************************
 * An implementation for SOR+GS method, see [Mittal(2014) S Mittal.
 * A study of successive over-relaxation method parallelisation over modern
 * HPC languages. IJHPCN. 7(4):292, 2014. doi: 10.1504/ijhpcn.2014.062731].
 ******************************************************************************/
template<u8 nd>
f32 run_SOR_iter (poisson_eq_t<nd> & eq, f32 w) {
	
	f32 verr{0.0f}, vold, vnew, diff;
	
	/* loop over red/black-units & perform SOR-step */
	for (u32 seq{red_black_seq(nd)}; seq; seq = seq >>(nd+1)) {
		
		u64 _offst[nd+1]; _offst[nd] = 1;
		for (u8 i{1u}; i<=nd; ++i) {
			_offst[nd-i] = (eq.shape[nd-i] - (1&(seq>>(nd-i))) + 1)/2;
			_offst[nd-i] = _offst[nd-i]*_offst[nd-i+1];
		}
		
		#pragma omp parallel for reduction(max:verr) private(vold, vnew, diff)
		for (u64 k=0; k<_offst[0]; ++k) {
			u32 pos[nd];
			u64 rem{k}, uid{0};
			for (u8 i{0u}; i<nd; ++i) {
				pos[i] = 2*(rem/_offst[i+1]) + (1&(seq>>i));
				uid = uid + pos[i]*eq.offst[i+1];
				rem = rem%_offst[i+1];
			}

			vold = eq.vdata[uid];
			vnew = eq.get_vnew(pos);
			vnew = w*vnew + (1.0f-w)*vold;
			diff = fabsf(vnew - vold);
			
			if (isfinite(vnew)) [[likely]] {
				verr = diff > verr ? diff : verr;
			}
			eq.vdata[uid] = vnew;
		} /* end parallel loop */ 
		
	}
	
	return verr;
}

#include "run_poisson_eq_fns.cxx"




