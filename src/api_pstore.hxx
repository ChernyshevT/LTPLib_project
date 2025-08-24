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
	size_t buffer_pos(u32 nadd) const {
		u32 j1 = npmax-nadd;
		return (j1 >= index[0]) ? j1 : 0;
	}
};
////////////////////////////////////////////////////////////////////////////////
struct pstore_t {
	/* particle data [{tag[4], x,y,z,vx,vy,vz,extra}, ...] */
	part_t *ppdata;
	/* [num parts, {num leaving}...] */
	u32    *pindex;
	/* [num holes, {p-ids}...] */
	u32    *pflags;
	/* load balance */
	u32    *queue;  
	/* charge-to-mass ratios */
	f32    *cffts;

	size_t npmax, nargs;

	struct {
		u8 ongpu :1;
		u8 fcache:3;
		u8 ntypes:4;
		u8 idshift;
		u8 mode;
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
