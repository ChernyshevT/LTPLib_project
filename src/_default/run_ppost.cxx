#define BACKEND_DEBUG

#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/

#include "api_backend.hxx"
#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "api_pstore.hxx"
#include "common/pushers.hxx"
#include "common/loop_over.hxx"

template<u8 nd, u8 mode, u8 ord>
u32 run_ppost
(const grid_t<nd> &grid, const pstore_t &pstore, vcache_t<f32> &latt) {

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
			/* skip if particle is adsorbed (in case of implicit solver) */
			if (node.check_mask(form.idx)) [[unlikely]] {
				continue;
			}
			
			// write pVDF moments
			auto fn = [&p, &cache] (f32 w, size_t kk) {
				const size_t midx = kk+p.tag[0]*mode;
				// write density
				if constexpr (mode >= POST_MODE::C) {
					cache[midx] += w;
				}
				// write flux
				if constexpr (mode >= POST_MODE::CF) {
					cache[midx+1] += w*p.vel[0]; //vx
					cache[midx+2] += w*p.vel[1]; //vy
					cache[midx+3] += w*p.vel[2]; //vz
				}
				// write pressure (raw)
				if constexpr (mode >= POST_MODE::CFP) {
					cache[midx+4] += w*p.vel[0]*p.vel[0]; //vx*vx
					cache[midx+5] += w*p.vel[1]*p.vel[1]; //vy*vy
					cache[midx+6] += w*p.vel[2]*p.vel[2]; //vz*vz
				}
				//write stress (raw)
				if constexpr (mode >= POST_MODE::CFPS) {
					cache[midx+7] += w*p.vel[0]*p.vel[1]; //vx*vy
					cache[midx+8] += w*p.vel[0]*p.vel[2]; //vx*vz
					cache[midx+9] += w*p.vel[1]*p.vel[2]; //vy*vz
				}
			};
			loop_over_form<ord+1,nd>(fn, offst, form.idx, form.vals);
		}
		// end loop over particles
	}
	// end omp parallel for

	return flags;
}

#include "run_ppost_fns.cxx"
