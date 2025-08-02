#pragma once
#include "typedefs.hxx"
#include "api_backend.hxx"

struct post_fn {
	f32 *vpart;
	f32 *vdata;
	u64  fcode;
	u64  shift;
	
	inline
	post_fn (u8 _tag, f32 _vpart[], f32 _vdata[], u64 _fcode) {
		fcode = _fcode;
		shift = (_fcode & 0xf) * _tag;
		vpart = _vpart;
		vdata = _vdata;
	}
	
	inline
	void operator () (f32 w, u64 k) {
		f32 vx{vpart[0]}, vy{vpart[1]}, vz{vpart[2]};
		
		for (u64 i{0u}, n{0xf & fcode}; i<n; ++i) switch (0xf & (fcode>>(4*i+4))) {
			default:
				break;
			case PPOST_ENUM::C0:
				vdata[k + shift+i] += w;
				continue;
			case PPOST_ENUM::Fx:
				vdata[k + shift+i] += w*vx;
				continue;
			case PPOST_ENUM::Fy:
				vdata[k + shift+i] += w*vy;
				continue;
			case PPOST_ENUM::Fz:
				vdata[k + shift+i] += w*vz;
				continue;
			case PPOST_ENUM::Pxx:
				vdata[k + shift+i] += w*vx*vx;
				continue;
			case PPOST_ENUM::Pyy:
				vdata[k + shift+i] += w*vy*vy;
				continue;
			case PPOST_ENUM::Pzz:
				vdata[k + shift+i] += w*vz*vz;
				continue;
			case PPOST_ENUM::Pxy:
				vdata[k + shift+i] += w*vx*vy;
				continue;
			case PPOST_ENUM::Pxz:
				vdata[k + shift+i] += w*vx*vz;
				continue;
			case PPOST_ENUM::Pyz:
				vdata[k + shift+i] += w*vy*vz;
				continue;
			case PPOST_ENUM::KEn:
				vdata[k + shift+i] += w*(vx*vx + vy*vy + vz*vz);
				continue;
		}
	};
};
