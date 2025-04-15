#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/
#pragma once
#include "typedefs.hxx"
#include <tgmath.h>
#include <math.h>

/******************************************************************************/
//@LISTING{start:scatter}
// Scatters particle relative to it's original direction.
// w1[] : new direction
// w0[] : initial direction
// deflection angles: $\sin\alpha$, $\cos\alpha$, $\sin\beta$, $\sin\beta$
inline void scatter \
(f32 w1[], const f32 w0[], f32 sina, f32 cosa, f32 sinb, f32 cosb) {
	f32 s;
	if (fabs(w0[2]) < 1.0f) {
		s = sqrtf(1.0f - w0[2]*w0[2]);
		w1[0] = w0[0]*cosa + (w0[0]*w0[2]*cosb - w0[1]*sinb)*sina/s;
		w1[1] = w0[1]*cosa + (w0[1]*w0[2]*cosb + w0[0]*sinb)*sina/s;
		w1[2] = w0[2]*cosa - sina*cosb*s;
	} else {
		s = 0.0f;
		w1[0] = sina*cosb;
		w1[1] = sina*sinb;
		w1[2] = cosa;
	}
	#if defined(BACKEND_DEBUG) || defined(FUNC_DEBUG)
	bool _debug_check{false};
	for (u8 i{0u}; i<3; ++i) {
		_debug_check or_eq (w1[i] != w1[i] or w0[i] != w0[i]);
	}
	if (_debug_check) {
		#pragma omp critical
		{
			setbuf(stdout, nullptr);
			printf("bad sample after scatter");
			printf(" (sinα = %+5.2f", sina);
			printf(", cosα = %+5.2f", cosa);
			printf(", sinβ = %+5.2f", sinb);
			printf(", cosβ = %+5.2f", cosb);
			printf(", s = %5.3e)\n", s);
			for (u8 i{0u}; i<3; ++i) {
				printf("w%c %+10.3e -> %+10.3e\n", char('x'+i), w0[i], w1[i]);
			}
		}
	}
	#endif
}
//@LISTING{end:scatter}
