#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "api_pstore.hxx"
#include "common/pushers.hxx"
#include "common/loop_over.hxx"

#include "api_backend.hxx"
#include "common/mcc_check.hxx"
#include "common/collisions.hxx"

template<u8 nd>
u32 run_mcsim
(const grid_t<nd> &grid, pstore_t &pstore, vcache_t<u32> &cflatt
, const csection_set_t &cset, const vcache_t<f32> &bglatt, f32 dt, u32 seed) {
	
	u32 flags{0};
	#pragma omp parallel for
	for (u32 k=0; k<grid.size; ++k) {

		auto node{grid[k]};
		auto pool{pstore[k]};
		auto bgseg{bglatt[k]};
		auto cfseg{cflatt[k]};
		auto rng{rng_t(seed+k)};
		u32 j1{pool.index[0]}, nh{0};
		for (u32 j{0}; j<j1; ++j) {
			struct {
				part_t data[1 + (nd+3)];
				u8  *tag =  data[0].tag;
				f32 *pos = &data[1].vec;
			} p0, p1;
			u32 idx[nd];
			
			for (auto i{0}; i<1+nd+3; ++i) {
				p0.data[i] = pool.parts[j][i];
			}
			
			u32 flag{0};
			for (u8 i{0u}; i<nd; ++i) {
				idx[i] = u32((p0.pos[i]-node.edgel[i])/grid.step[i]);
				flag |= ERR_CODE::OUTOFRANGE*(idx[i] >= node.shape[i]);
			}
			if (flag) {
				#pragma omp atomic
				flags |= flag;
				
				continue;
			}
			
			f32 *bg{bgseg};
			for (auto i{1u}, sh{bglatt.vsize}; i<=nd; ++i) {
				bg += idx[nd-i]*sh;
				sh *= node.shape[nd-i];
			}
			
			auto cl{mcrun(rng, cset, p0.tag[0], p0.pos+nd, bg, dt)};
			next: switch (cl.type) {
				
				case cltype::ERROR_ENLIMIT:
					#pragma omp atomic
					flags |= ERR_CODE::ENERGYMAX;
					
				goto skip;
				
				case cltype::ERROR_PROBMAX:
					#pragma omp atomic
					flags |= ERR_CODE::PROBMAX;
					
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
						flags |= ERR_CODE::OVERFLOW;
						
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
				u32 *cf{cfseg};
				for (auto i{1u}, sh{cflatt.vsize}; i<=nd; ++i) {
					cf += idx[nd-i]*sh;
					sh *= node.shape[nd-i];
				}
				++cf[cl.chnl-1];
			}
		} // end loop over particles
		
		// fill up holes with particles from the tail
		for (u32 ih{0}, j_src, j_dst; ih<nh; ++ih) {
			j_src = j1-ih-1;
			j_dst = pool.flags[(nh-ih-1)*2+1];
			if (j_src > j_dst) for (size_t i{0}; i<pool.nargs; ++i) {
				pool.parts[j_dst][i] = pool.parts[j_src][i];
			}
		}
		// write actual particle number
		pool.index[0] = j1-nh;
	}
	// end omp parallel for

	return flags;
}

/*
#define EXPORT_PMCSIM_FN(N) \
extern "C" LIB_EXPORT \
RET_ERRC mcsim##N##_fn ( \
const grid_t<N> &grid, pstore_t &pstore, vcache_t<u32> &cfreq, \
const csection_set_t &cset, const vcache_t<f32> &bg, f32 dt, u32 seed \
) {return run_mcsim<N>(grid, pstore, cfreq, cset, bg, dt, seed);}

EXPORT_PMCSIM_FN(1)
EXPORT_PMCSIM_FN(2)
EXPORT_PMCSIM_FN(3)
*/
