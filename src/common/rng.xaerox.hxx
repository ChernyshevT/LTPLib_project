#pragma once
#include "typedefs.hxx"

/* Deprecated! I decided to not use it due to license limitations. */ 

class rng_t {
/* Fast uniform pseudorandom generator from
 * http://hlfx.ru/forum/showthread.php?s=&threadid=4247&postid=131218#post131218 */
// Random generator, author: XaeroX, 2014
// Part of Volatile3D II Engine source code
// Code license:
// Creative Commons Attribution-Noncommercial-Share Alike
// http://creativecommons.org/licenses/by-nc-sa/4.0/

// could be local statics, but we reference them
// in two subroutines: srand and rand
// the initial values are arbitrary unsigned ints
private:
	unsigned int
		awc0 = 0xf6a3d9,
		awc1 = 0x159a55e5,
		mult = 0x436cbae9;
	unsigned int carry  = 1;
	//for normal distro
	f32        Z1, Z2;
	unsigned int cached = 0;

	inline
	void xrxSRand( unsigned int seed ) {
		// initialize AWC and multiplicative seeds
		awc0 = 0x10dcd * seed + 0xf6a3d9;
		awc1 = 0x10dcd * seed + 0x159a55e5;
		mult = seed & 0x7fffffff;

		// carry flag initial value depends on awc seeds
		carry = ( awc0 > awc1 ) ? 0 : 1;
	}

	inline
	unsigned int xrxRand( void ) {
		// run AWC-generator
		unsigned int awc = awc1 + ( awc0 + carry );
		carry = ( awc > awc0 ) ? 0 : 1;
		awc1 = awc0; awc0 = awc;

		// run multiplicative generator
		mult = 0x10dcd * mult + 0x3c6ef373;

		// return random value
		return awc + mult;
	}

	inline
	f32 Com_RandomFloat( f32 low, f32 high ) {
		f32 randNumber;
		f32 randRange = high - low;

		if ( randRange <= 0 )
			return low;

		randNumber = 0.2328306437e-9f * xrxRand();
		return low + randNumber * randRange;
	}
//****************************************************************************//
public:

	inline
	rng_t(unsigned int seed=0) {
		xrxSRand(seed);
	}
	
/**********************************************************************/

	inline
	f32 uniform( f32 a=1.0f, f32 b=0.0f ) {
		return (b<a) ? Com_RandomFloat(b, a) : Com_RandomFloat(a, b);
	}
/**********************************************************************/

	inline
	f32 normal( f32 s=1.0f, f32 m=0.0f ) {
		// Box-Muller transform
		if (cached) {
			cached = 0;
			return s*Z2 + m;
		} else {
			f32 R1, R2, R;
			do {
				R1 = uniform(-1.0f, 1.0f);
				R2 = uniform(-1.0f, 1.0f);
				R  = R1*R1 + R2*R2;
			} while (R>1.0f or R<=FLT_EPSILON);
			Z1 = R1*sqrtf(-2.0f*logf(R)/R);
			Z2 = R2*sqrtf(-2.0f*logf(R)/R);
			cached = 1;
			return Z1*s + m;
		}
	}
};
