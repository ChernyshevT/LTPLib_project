#pragma once
#include "typedefs.hxx"

#ifdef XRXRNG
#pragma message ("using xrx_rng")
#include "rng.xaerox.hxx"
#else
#pragma message ("using kiss_rng")
struct rng_t {
// Fast uniform pseudorandom generator based on KISS-rng,
// see [https://github.com/sleeepyjack/kiss_rng]
	inline
	rng_t (u32 seed) : w{seed != 0 ? seed : 0xFFFFFFFF}, cache{0.0f} {
		auto _hashify = [] (u32 v) -> u32 {
			// MurmurHash
			v ^= v >> 16;
			v *= 0x85ebca6b;
			v ^= v >> 13;
			v *= 0xc2b2ae35;
			v ^= v >> 16;
			return v;
		};
		//
		for (u8 i{0}; i<8; ++i) {
			x = _hashify(w);
			y = _hashify(x);
			z = _hashify(y);
			w = _hashify(z);
		}
	}
	
	inline
	u32 next (void) {
		// lcg
		x = 69069 * x + 12345;
		// xorshift
		y ^= y << 13;
		y ^= y >> 17;
		y ^= y << 5;
		// carry and multiply
		u64 t = 698769069ULL * z + w;
		w = t >> 32;
		z = u32(t);
		// combine
		return x + y + z;
	}
	
	inline
	f32 uniform (f32 a, f32 b) {
		return a + (b-a)*(next()*0.2328306437e-9f);
	}
	
	inline
	f32 normal(f32 s=1.0f, f32 m=0.0f) {
		f32 value;
		// Box-Muller transform
		if (cache != 0) {
			value = cache;
			cache = 0.0f;
		} else {
			f32 R1, R2, R;
			do {
				R1 = uniform(-1.0f, 1.0f);
				R2 = uniform(-1.0f, 1.0f);
				R  = R1*R1 + R2*R2;
			} while (R > 1.0f or R <= FLT_EPSILON);
			value = R1*sqrtf(-2.0f*logf(R)/R);
			cache = R2*sqrtf(-2.0f*logf(R)/R);
		}
		return value*s + m;
	}
	
private:
	u32 w, x, y, z;
	f32 cache;
};
#endif
