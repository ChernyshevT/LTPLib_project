#include "api_backend.hxx"

template<u8 nd, u8 md=nd>1? nd>2? 27: 9: 3>
u32 run_order
(const grid_t<nd>& grid, pstore_t & pstore) {

	u32 flags{0};
	#pragma omp parallel for
	for (u32 k=0; k<grid.size; ++k) {
		auto dst = pstore[k];
		u32 ih{0}, nh{dst.flags[0]}, npp{dst.index[0]}, j_src, j_dst;

		// loop over particles incoming from nearby buffers
		for (u8 idir=1; idir<md; ++idir) if (grid.nodes[k].lnk[idir-1]) {
			auto src = pstore[grid.nodes[k].lnk[idir-1]-1];

			for (j_src=src.index[idir]; j_src<src.index[idir+1]; ++j_src) {
				if (ih < nh) {
					j_dst = dst.flags[ih*2+1];
					++ih;
				} else {
					j_dst = npp;
					++npp;
				}

				// check overflow & insert incoming particle into hole/tail
				if (j_dst < dst.index[1]) for (size_t i{0}; i<dst.nargs; ++i) {
					dst.parts[j_dst][i] = src.parts[j_src][i];
				} else {
					#pragma omp atomic
					flags |= ERR_CODE::OVERFLOW;
				}
			}
		}

		// fill up remaining holes with particles from the tail
		for (u32 j=0; j<nh-ih; ++j) {
			j_src = npp-j-1;
			j_dst = dst.flags[(nh-j-1)*2+1];
			if (j_src > j_dst) for (size_t i{0}; i<dst.nargs; ++i) {
				dst.parts[j_dst][i] = dst.parts[j_src][i];
			}
		}

		// write actual particle number
		dst.index[0 ] = npp-nh+ih;
	}
	// end omp parallel for
	
	return flags;
}

