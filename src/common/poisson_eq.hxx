#pragma once
#include "typedefs.hxx"

/***************** node zz xx yy **********************************************/
#define SETVALUE   0b01'00'00'00 // boundary condition (Dirichlet)
#define SETAXIS    0b10'00'00'00 // mark finite differences
#define LFDIFF     0b00'00'00'01 // left finite difference
#define RTDIFF     0b00'00'00'10 // right finite difference
#define CNDIFF     0b00'00'00'11 // central finite difference (left+right)

#define CHECK_UNIT(arg)     (0b11'00'00'00 &  (arg))
#define CHECK_AXIS(arg, ax) (0b00'00'00'11 & ((arg) >> ((ax) * 2)))

/******************************************************************************/
template<u8 nd>
struct poisson_eq_t {
	u64  offst[nd+1]; // nx*ny*nz*1
	u32  shape[nd];
	f32  dstep[nd];   // 1/dx/dx
	u8  *umap;        // u[nit] map
	f32 *cdata;       // c[charge] data
	f32 *vdata;       // v[oltage] data
	
	/* pos: ix,iy,iz; nseq: red (0) -> black (1) */
	inline f32 get_vnew (u32 pos[nd]) {
		f32  vnew{0.0f}, cfft{0.0f};
		
		// mid-point, left, right indexes
		u64 idpt{0}, idlf, idrt;
		i32 shlf, shrt, nlen;
		for (u8 i{0u}; i<nd; ++i) {
			idpt += offst[i+1] * pos[i];
		}

		/* check the unit to perfornm the action **********************************/
		u8 ucode{umap[idpt]};
		switch CHECK_UNIT(ucode) {

			default:
				vnew = NAN;
				break;

			case SETVALUE:
				vnew = vdata[idpt];
				break;

			// loop over each axis & update vnew
			case SETAXIS: [[likely]]
				for (u8 j{0u}; j<nd; ++j) {
					idlf = 0; idrt = 0;
					for (u8 i{0u}; i<nd; ++i) {
						nlen = shape[i];
						shlf = pos[i] - (i==j)*(CHECK_AXIS(ucode,j)&LFDIFF? 1 : -1);
						shrt = pos[i] + (i==j)*(CHECK_AXIS(ucode,j)&RTDIFF? 1 : -1);
						
						idlf += offst[i+1] * ((shlf%nlen + nlen) % nlen);
						idrt += offst[i+1] * ((shrt%nlen + nlen) % nlen);
					}
					vnew += (vdata[idlf]+vdata[idrt])*dstep[j];
					cfft += 2*dstep[j];
				}
				vnew = (vnew-cdata[idpt])/cfft;
				break;
		}
		return vnew;
	} 
};


