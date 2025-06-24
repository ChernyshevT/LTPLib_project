#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "api_pstore.hxx"
#include "common/pushers.hxx"
#include "common/loop_over.hxx"

#include "run_order.cxx"

template<u8 nd, u8 mode, u8 ord, u8 cylcrd=0>
u32 run_ppush
(const grid_t<nd> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	
	constexpr int md{nd>1 ? nd>2 ? 27 : 9 : 3};
	
	constexpr u8
		nread = mode<1 ? mode<2 ? 1+nd+3 : 1+nd+3     : 1+(nd+3)*2,
		nbuff = mode<1 ? mode<2 ? 1+nd+3 : 1+(nd+3)*2 : 1+(nd+3)*2;

	f32 mudt[16];

	for (auto i{0u}; i<pstore.opts.ntypes; ++i) {
		mudt[i] = pstore.cffts[i]*dt*0.5f;
	}
	dt = mode ? dt*0.5 : dt;

	//////////////////////////////////////////////////////////////////////////////
	u32 flags{0};
	#pragma omp parallel for
	for (u32 k=0; k<grid.size; ++k) {

		auto node   = grid[k];
		size_t  offst[nd+1]; offst[nd]=field.vsize;
		for (int i=nd-1; i>=0; --i) {
			offst[i] = offst[i+1]*(node.shape[i] + ord);
		}

		u32 ncl[md+1]{0}, nh{0};
		for (u8 i{0}; i<=md; ++i) {
			ncl[i] = 0;
		}
		auto pool   = pstore[k];
		auto flocal = field[k]; // link to local field

		////////////////////////////////////////////////////////////////////////////
		// loop over particles
		for (u32 j{0}; j<pool.index[0]; ++j) {

			struct {
				part_t data[nbuff];
				u8  *tag =  data[0].tag;
				f32 *pos = &data[1].vec;
			} p;
			form_t<nd, ord> form;

			//read particle data from the pool
			for (size_t i{0}; i<nread; ++i) {
				p.data[i] = pool.parts[j][i];
			}

			/* find local position & form-factor, check out of range */
			if (auto flag{node.get_form(&form, p.pos)}; flag) {
				#pragma omp atomic
				flags |= flag;
				
				continue;
			}
			
			/* check for absorbers (if present) */
			if constexpr (mode < PUSH_MODE::IMPLR) {
				if (node.check_mask(form.idx)) [[unlikely]] {
					pool.flags[nh*2+1] = j;
					pool.flags[nh*2+2] = 0;
					++nh;
					continue;
				}
			}

			/* obtain local field acting on particle ***************/
			f32 fpt[6]{0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
			auto fn = [&fpt, &flocal, &fcode] (f32 w, size_t k) {
				for (u32 i{0u}, n{0xf & fcode}; i<n; ++i) {
					fpt[0xf & (fcode>>(4*i+4))] += w*flocal[k+i];
				}
			};
			loop_over_form<ord+1,nd>(fn, offst, form.idx, form.vals, mudt[p.tag[0]]);

			// push particle (t -> t+dt)
			push_pt<nd, mode, cylcrd>(p.pos, fpt, dt);
			
			// find the direction
			if (u8 idir{node.find_idir(p.pos, mode>0 ? p.pos+nd+3 : nullptr)}) {
				pool.flags[nh*2+1] = j;
				pool.flags[nh*2+2] = idir;
				++nh;
				++ncl[idir+1];
			}

			// write particle back into pool
			for (size_t i=0; i<nbuff; ++i) {
				pool.parts[j][i] = p.data[i];
			}
		}
		pool.flags[0] = nh;
		// end loop over particles
		////////////////////////////////////////////////////////////////////////////
		
		// check overflow and move particles into the pool's tail
		size_t nmov{0};
		for (u8 i{2}; i<=md; ++i) {
			nmov += ncl[i];
		}
		ncl[0] = pool.buffer_pos(nmov);
		if (ncl[0] != 0) {
			// determine shifts to move particles
			for (size_t i{1}; i<=md; ++i) {
				ncl[i] += ncl[i-1];
			}
			// loop over holes & move particles into buffer
			for (size_t ih{0}, j1, id, j2; ih<nh; ++ih) {
				j1 = pool.flags[ih*2+1];
				id = pool.flags[ih*2+2];
				if (id > 0) {
					j2 = ncl[id]; ++ncl[id];
					for (size_t i{0}; i<nbuff; ++i) {
						pool.parts[j2][i] = pool.parts[j1][i];
					}
				}
			}
			// update index
			for (size_t i{1}; i<=md; ++i) {
				pool.index[i]=ncl[i-1];
			}
		} else {
			#pragma omp atomic
			flags |= ERR_CODE::PTOVERFLOW;
		}
	}
	// end omp parallel for
	
	return flags;
}

#include "run_ppush_fns.cxx"


