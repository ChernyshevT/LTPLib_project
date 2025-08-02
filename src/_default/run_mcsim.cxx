#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "api_pstore.hxx"
#include "common/loop_over.hxx"
#include "common/mcc_check.hxx"
#include "common/collisions.hxx"

template<u8 nd>
u32 run_mcsim
(const grid_t<nd> &grid, pstore_t &pstore, vcache_t<u32> &events
, const csection_set_t &cset, const vcache_t<f32> &bgrnd, f32 dt, u32 seed) {
	
	/* loop over nodes */
	u32 flags{0};
	#pragma omp parallel for
	for (u32 k=0; k<grid.size; ++k) {
		auto rng{rng_t(seed+k)};
		auto node{grid[k]};
		auto pool{pstore[k]};
		
		f32 *bgrnd_seq{bgrnd[k]};
		u32 *event_seq{events[k]};
		
		/* loop over particles */
		u32 j1{pool.index[0]}, nh{0};
		for (u32 j{0}; j<j1; ++j) {
			struct {
				part_t data[1 + (nd+3)];
				u8  *tag =  data[0].tag;
				f32 *pos = &data[1].vec;
			} p0, p1;
			
			for (auto i{0}; i<1+nd+3; ++i) {
				p0.data[i] = pool.parts[j][i];
			}
			
			/* obtain position id */
			u32 flag{0}, idpt{0};
			for (u32 i{1u}, sh{1u}, axpos; i<=nd; sh*=node.shape[nd-i], ++i) {
				axpos = u32((p0.pos[nd-i]-node.edgel[nd-i])/grid.step[nd-i]);
				idpt += axpos*sh;
				flag |= ERR_CODE::PTOUTOFRANGE*(axpos >= node.shape[nd-i]);
			}
			if (flag) {
				#pragma omp atomic
				flags |= flag;
				
				continue;
			}
			
			/* run Monte-Carlo simulation */
			auto cl{mcrun(rng, cset, p0.tag[0], p0.pos+nd, &bgrnd_seq[idpt*bgrnd.vsize], dt)};
			next: switch (cl.type) {
				
				case cltype::ERROR_ENLIMIT:
					#pragma omp atomic
					flags |= ERR_CODE::PTMAXENERGY;
					
				goto skip;
				
				case cltype::ERROR_PTMAXPROBABILITY:
					#pragma omp atomic
					flags |= ERR_CODE::PTMAXPROBABILITY;
					
				goto skip;
				
				// collisions
				case cltype::CONSERVATIVE:
					cl.do_conservative(p0.pos+nd);
					for (auto i{0}; i<1+nd+3; ++i) {
						pool.parts[j][i] = p0.data[i];
					}
				goto end;
				
				// TODO:
				// spawn heavy secodnary particle
				// sample from temperature
				// transform victim from the current pool (reqire sorting or second run)
				case cltype::IONIZATIONRUN: // swapn same secondary particles
					if (j1 == pool.npmax) {
						#pragma omp atomic
						flags |= ERR_CODE::PTOVERFLOW;
						
						goto skip;
					}
					cl.do_ionization_run(p0.pos+nd);
					for (auto i{0}; i<1+nd+3; ++i) {
						pool.parts[j1][i] = p0.data[i];
					}
					j1++;
				goto next;
				
				case cltype::IONIZATIONEND: // incident particle
					cl.do_ionization_end(p0.pos+nd);
					for (auto i{0}; i<1+nd+3; ++i) {
						pool.parts[j][i] = p0.data[i];
					}
				goto end;
				
				case cltype::ATTACHMENT:
					pool.flags[(nh++)*2+1]=1;
				goto end;
				
				// cltype::NONE 
				default: goto end;
				
			}
			skip: continue;
			
			end: if (cl.chnl) {
				++event_seq[idpt*events.vsize + (cl.chnl-1)];
			}
		}
		/* end loop over particles */
		
		/* fill up holes with particles from the tail */
		for (u32 ih{0}, j_src, j_dst; ih<nh; ++ih) {
			j_src = j1-ih-1;
			j_dst = pool.flags[(nh-ih-1)*2+1];
			if (j_src > j_dst) for (size_t i{0}; i<pool.nargs; ++i) {
				pool.parts[j_dst][i] = pool.parts[j_src][i];
			}
		}
		/* write actual particle number */
		pool.index[0] = j1-nh;
	}
	/* end loop over nodes */

	return flags;
}

#include "run_mcsim_fns.cxx"
