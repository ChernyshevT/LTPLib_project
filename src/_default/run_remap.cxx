#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "common/loop_over.hxx"
#include "api_backend.hxx"

namespace {
	
	template<u8 nd, typename tp>
	void run_remap_to_nodes
	(const grid_t<nd> &grid, vcache_t<tp> &latt, tp *g_data ) {
		
		size_t g_shape[nd];
		size_t g_offst[nd+1]; g_offst[nd]=latt.vsize;
		for (int i=nd-1; i>=0; --i) {
			g_shape[i] = grid.axes[i][grid.shape[i]]+latt.order;
			g_offst[i] = g_shape[i]*g_offst[i+1];
		}

		////////////////////////////////////////////////////////////////////////////
		#pragma omp parallel for schedule(dynamic)
		for (u32 k=0; k<grid.size; ++k) {
			size_t   l_offst[nd+1]; l_offst[nd]=latt.vsize;
			size_t   l_shape[nd];
			size_t   l_range[nd*2];
			size_t   g_shift[nd];
			for (int i=nd-1; i>=0; --i) {
				size_t j = grid.nodes[k].map[i];
				size_t a = grid.axes[i][j];
				size_t b = grid.axes[i][j+1];
				
				g_shift[i] = a;
				l_shape[i] = latt.order + b-a;
				l_offst[i] = l_offst[i+1]*l_shape[i];
				l_range[i*2  ] = 0;
				l_range[i*2+1] = l_shape[i];
			}
			
			tp *l_data = latt[k];
			auto fn = [&l_data, &g_data, &l_offst] (size_t gk, size_t ck) {
				for (size_t i=0; i<l_offst[nd]; ++i) {
					l_data[ck+i] = g_data[gk+i];
				}
			};
			loop_over_block<nd>(fn, g_offst, g_shift, l_offst, l_range);
		}
		////////////////////////////////////////////////////////////////////////////
	}

	/******************************************************************************/
	template<u8 nd, typename tp>
	void run_remap_to_array
	(const grid_t<nd> &grid, vcache_t<tp> &latt, tp *g_data ) {
		
		size_t g_shape[nd];
		size_t g_offst[nd+1]; g_offst[nd]=latt.vsize;
		for (int i=nd-1; i>=0; --i) {
			g_shape[i] = grid.axes[i][grid.shape[i]] + latt.order;
			g_offst[i] = g_shape[i]*g_offst[i+1];
		}

		////////////////////////////////////////////////////////////////////////////
		#pragma omp parallel for schedule(static)
		for (size_t i=0; i<g_offst[0]; ++i) {
			g_data[i] = 0;
		}
		////////////////////////////////////////////////////////////////////////////

		// loop over $2^{nd}$ corners to avoid race condition
		constexpr u64 fmask[]={ // see test_drafts/test_cube.py
			0,
			0b1001,
			0b110101011001,
			0b11100111101100111101010110010001,
		};
		for (u64 flag{fmask[nd]}; flag; flag >>= nd+1) {
			//////////////////////////////////////////////////////////////////////////
			#pragma omp parallel for schedule(dynamic)
			for (u32 k=0; k<grid.size; ++k) {
				size_t   l_offst[nd+1]; l_offst[nd]=latt.vsize;
				size_t   l_shape[nd];
				size_t   l_range[nd*2];
				size_t   g_shift[nd];
				for (int i=nd-1; i>=0; --i) {
					size_t j = grid.nodes[k].map[i];
					size_t a = grid.axes[i][j];
					size_t b = grid.axes[i][j+1];
					
					g_shift[i] = a;
					l_shape[i] = b-a + latt.order;
					l_offst[i] = l_offst[i+1]*l_shape[i];
					
					if ( not ((1<<(i+1)) & flag) ) {
						// base segment
						l_range[i*2  ] = 0;
						l_range[i*2+1] = l_shape[i]-latt.order;
					} else {
						// guard cells
						l_range[i*2  ] = l_shape[i]-latt.order;
						l_range[i*2+1] = l_shape[i];
					}
				}
				
				tp *l_data = latt[k];
				auto fn = [&l_data, &g_data, &l_offst] (size_t gk, size_t ck) {
					for (size_t i=0; i<l_offst[nd]; ++i) {
						g_data[gk+i] += l_data[ck+i];
					}
				};
				loop_over_block<nd>(fn, g_offst, g_shift, l_offst, l_range);
			}
			//////////////////////////////////////////////////////////////////////////
		}
		auto fn = [&data=g_data] (size_t j0, size_t j1, size_t i) {
			data[j0+i] += data[j1+i];
			data[j1+i]  = data[j0+i];
		};
		for ( int i=0; i<nd; ++i ) if ( 1&(grid.flags.loopax>>i) ) {
			loop_over_bound<nd>(fn, i, latt.order, g_offst, g_shape);
		}
	}
	
}
#include "run_remap_fns.cxx"


