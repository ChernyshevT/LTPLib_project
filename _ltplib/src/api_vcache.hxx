#ifndef _COM_LATTICES_HEADER
#define _COM_LATTICES_HEADER

template<typename tp>
struct vcache_t {
	tp  *data;
	u64 *shift;
	u32  blocksize;
	u32  vsize;
	u8   order;

	inline
	tp* operator [] (u32 k) const {
		return data+k*blocksize;
	}
};

// https://medium.com/@saleem.iitg/c-compile-time-conditional-struct-member-variables-c3761a35eca9

#endif
