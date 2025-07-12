#define BACKEND_DEBUG

#ifdef BACKEND_DEBUG
#include <cstdio>
#include <format>
#include <iostream>
#endif
/******************************************************************************/

#include "api_backend.hxx"
#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "api_pstore.hxx"
#include "common/pushers.hxx"
#include "common/loop_over.hxx"
#include "common/post_fn.hxx"

template<u8 nd, u8 ord>
u32 run_ppost
(const grid_t<nd> &grid, const pstore_t &pstore, vcache_t<f32> &latt, u64 fcode) {

	u32 flags{0};
	#pragma omp parallel for
	for (u32 k=0; k<grid.size; ++k) {
		
		auto node = grid[k];
		//size_t shift[nd];
		size_t offst[nd+1]; offst[nd]=latt.vsize;
		for (int i=nd-1; i>=0; --i) {
			offst[i] = offst[i+1]*(node.shape[i]+ord);
		}
		// clean cache
		f32 *cache = latt[k];
		for (size_t i=0; i<offst[0]; ++i) {
			cache[i] = 0.0f;
		}

		// loop over particles
		auto pool = pstore[k];
		for (size_t j{0}; j<pool.index[0]; ++j) {

			struct {
				part_t data[1 + (nd+3)*2];

				u8  *tag =  data[0].tag;
				f32 *pos = &data[1].vec;
				f32 *vel = &data[1+nd].vec;
			} p;
			form_t<nd, ord> form;
			
			for (size_t i=0; i<1+nd+3; ++i) {
				p.data[i] = pool.parts[j][i];
			}

			/* find local position & form-factor, check out of range */
			if (auto flag{node.get_form(&form, p.pos)}; flag) {
				#pragma omp atomic
				flags |= flag;
				
				continue;
			}
			
			/* skip sample if it is inside the masked cell */
			if (node.check_mask(form.idx)) [[unlikely]] {
				continue;
			}
			
			/* calculate pVDF moments */
			loop_over_form<ord+1,nd>(post_fn(p.tag[0], p.vel, cache, fcode)
			, offst, form.idx, form.vals);

		}
		// end loop over particles
	}
	// end omp parallel for

	return flags;
}

#include "run_ppost_fns.cxx"
