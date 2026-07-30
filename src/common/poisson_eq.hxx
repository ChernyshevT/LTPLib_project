#pragma once
#include "typedefs.hxx"

/***************** node zz yy xx **********************************************/
#define LFDIFF     0b00'00'00'01 // left finite difference
#define RTDIFF     0b00'00'00'10 // right finite difference
#define CNDIFF     0b00'00'00'11 // central finite difference (left+right)

#define CHECK_AXIS(arg, ax) (0b00'00'00'11 & ((arg) >> ((ax) * 2)))

/******************************************************************************/
template<u8 nd>
struct poisson_eq_t {
	u32  shape[nd];
	f32  dstep[nd+1]; // dx,dy,dz + ax-shift;
	u8  *umap;        // u[nit] map
	f32 *cdata;       // c[charge] data
	f32 *vdata;       // v[oltage] data
	
	inline u64 flat_index (u32 pos[]) const {
		u64 id{0}, sh{1};
		for (u8 i{1}; i<=nd; ++i) {
			id += sh * pos[nd-i];
			sh *= shape[nd-i];
		}
		return id;
	}
	
	/* pos: {ix,iy,iz}; red/black selection is done by the caller */
	inline f32 get_vnew (u32 pos[nd]) const {
		u64 cn = flat_index(pos);
		if (umap[cn] == 0) {
			return vdata[cn];
		}
		
		f32 vnew{0.0f}, cfft{0.0f}, dfrac, wl, wr;
		/* loop over stencil */
		for (u8 k{0}; k < nd; ++k) {
			u32 plf[nd]; u64 lf;
			u32 prt[nd]; u64 rt;
			for (u8 i{0}; i < nd; ++i) {
				plf[i] = pos[i];
				prt[i] = pos[i];
			}
			
			/* left neigbour (wrap in case of periodic boundary) */
			plf[k] = (pos[k]+shape[k]-1) % shape[k];
			lf = flat_index(plf);
			
			/* right neigbour (wrap in case of periodic boundary) */
			prt[k] = (pos[k]+shape[k]+1) % shape[k];
			rt = flat_index(prt);
			
			dfrac = 1.0f/(dstep[k]*dstep[k]);
			/* hack for axial symmetry if k-axis == 'y' */
			if (dstep[nd] >= 0.0f and k == 1 and nd == 2) {
				f32 r0 = dstep[nd] + pos[k]*dstep[k];
				//printf("p#[%03u,%03u]: CYLINDER, r0 = %f\n", pos[0], pos[1], r0);
				if (r0 > 0.0f) {
					wl = 1.0 - 0.5*dstep[k]/r0; 
					wr = 1.0 + 0.5*dstep[k]/r0;
				} else {
					wl = 1.0;
					wr = 1.0;
					lf = rt;
				}
			} else {
				wl = 1.0;
				wr = 1.0;
			}
			switch CHECK_AXIS(umap[cn], k) {
				default:
					return NAN;
				case LFDIFF: /* right open-boundary (E_{k} == 0) */
					vnew += (vdata[lf]*wl + vdata[cn]*wr)*dfrac;
					break;
				case RTDIFF: /* left open-boundary (E_ax] == 0) */
					vnew += (vdata[cn]*wl + vdata[rt]*wr)*dfrac;
					break;
				case CNDIFF: /* mid-point */
					vnew += (vdata[lf]*wl + vdata[rt]*wr)*dfrac;
					break;
			}
			cfft += 2*dfrac;
		}
		return (vnew-cdata[cn])/cfft;

	} 
};


