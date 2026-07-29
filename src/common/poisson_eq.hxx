#pragma once
#include "typedefs.hxx"

/***************** node zz yy xx **********************************************/
#define LFDIFF     0b00'00'00'01 // left finite difference
#define RTDIFF     0b00'00'00'10 // right finite difference
#define CNDIFF     0b00'00'00'11 // central finite difference (left+right)

#define CHECK_UNIT(arg)     (0b11'00'00'00 &  (arg))
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
		
		f32 vnew{0.0f}, cfft{0.0f};
		/* loop over stencil */
		for (u8 ax{0}; ax<nd; ++ax) {
			u32 plf[nd]; u64 lf;
			u32 prt[nd]; u64 rt;
			for (u8 i{0}; i<nd; ++i) {
				plf[i] = pos[i];
				prt[i] = pos[i];
			}
			
			/* left neigbour */
			plf[ax] = (pos[ax]+shape[ax]-1) % shape[ax];
			lf = flat_index(plf);
			
			/* right neigbour */
			prt[ax] = (pos[ax]+shape[ax]+1) % shape[ax];
			rt = flat_index(prt);
			
			switch CHECK_AXIS(umap[cn], ax) {
				default:
					return NAN;
				case LFDIFF: /* right open-boundary (E_{ax} == 0) */
					rt = cn;
					break;
				case RTDIFF: /* left open-boundary (E_ax] == 0) */
					lf = cn;
					break;
				case CNDIFF: /* mid-point */
					break;
			}
			vnew += (vdata[lf] + vdata[rt])*dstep[ax];
			cfft += 2*dstep[ax];
		}
		return (vnew-cdata[cn])/cfft;

	} 
};


