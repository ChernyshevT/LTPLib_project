#ifndef _COM_POOLS_HEADER
#define _COM_POOLS_HEADER
//~ #include "base.hxx"

////////////////////////////////////////////////////////////////////////////////
#include "typedefs.hxx"
union part_t {
	// tag[0..] -- i[x/y/z], tag[nd] -- ptype
	u8 tag[4]; 
	// vec[0..] -- r[x/y/z], vec[nd..] --v[x/y/z], forms[..], s0[x/y/z], u0[x/y/z]
	f32   vec; 
	
};
static_assert(sizeof(part_t) == sizeof(f32));


struct particle_t {
	part_t   *part;
	u32 sh;
	
	inline void readp (u8 n, u8 tag, f32 *vec) {
		
	}
	inline void write (u8 n, u8 tag, f32 *vec) {
		
	}
};

////////////////////////////////////////////////////////////////////////////////
struct pool_t {
	
	struct {
		part_t *pdata;
		size_t  poffset;
		
		inline
		part_t* operator [] (const size_t& k) const {
			return pdata + k*poffset;
		}
	} parts;
	
	u32 *index;
	u32 *flags;
	
	size_t npmax, nargs;
	
	inline
	size_t buffer_pos(size_t nadd) const {
		size_t j1 = npmax-nadd;
		return (j1 >= index[0]) ? j1 : 0;
	}
};
////////////////////////////////////////////////////////////////////////////////
struct pstore_t {
	part_t *ppdata;
	u32 *pindex;
	u32 *pflags;
	f32 *cffts;

	size_t npmax, nargs;
	//~ u32 nsamples;

	struct {
		u8 ongpu :1;
		u8 fcache:3;
		u8 ntypes:4;
		u8 idshift;
	} opts;
	
	//~ size_t count_samples (void) {
		//~ size_t n;
		//~ for 
	//~ } 
	
	inline
	pool_t operator [] (size_t k) const {
		return {
			{ppdata+k*nargs*npmax, opts.ongpu ? npmax : nargs},
			pindex+k*opts.idshift,
			pflags+k*(2*npmax+1),
			npmax,
			nargs
		};
	}

};

#endif
