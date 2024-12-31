#ifndef _BACKEND_IFACE_HEADER
#define _BACKEND_IFACE_HEADER

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT __attribute__ ((visibility ("default")))
#endif

#define API_V "API2024-11-23"

//~ #define STRING(s) #s
#include "typedefs.hxx"

template<u8 nd>
struct grid_t;

template<typename tp>
struct vcache_t;

struct pstore_t;

struct csection_set_t;

/******************************************************************************/

/*******************************************************************************
** RETURN VALUES **************************************************************/
enum ERR_CODE : u8 {
	SUCCESS     = 0x00,
	PPOST_ERR   = 0x01, // OUTOFRANGE
	PPUSH_ERR   = 0x02, // OVERFLOW|OUTOFRANGE|NANVALUE
	PORDER_ERR  = 0x04, // OVERFLOW|
	PMCSIM_ERR  = 0x08, // OVERFLOW|NANVALUE|OUTOFRANGE|ENERGYMAX|PROBMAX
	PCHECK_ERR  = 0x10, // OUTOFRANGE|NANVALUE|RUNAWAYPT|FASTPT
	INVALID_SEQ = 0xff, // something went wrong
};
#undef OVERFLOW
enum ERR_FLAG : u8 {
	NIL         = 0x00,
	OVERFLOW    = 0x01, // too many particles
	OUTOFRANGE  = 0x02, // particle lies outside of the pool or NaN occured
	NANVALUE    = 0x04, // something went really wrong...
	ENERGYMAX   = 0x08, // particle's collision energy > max_energy
	PROBMAX     = 0x10, // particle's collision probability > 0.25
	RUNAWAYPT   = 0x20, // particle jumps over half of the segment (log error)
	FASTPT      = 0x40, // particle jumps over the grid spacing    (log warning)
};

struct RET_ERRC {
	u8  code;
	u8  flags;
	f32 dtime;
};
 
/*******************************************************************************
** API for particles' form-factors ********************************************/

enum FORM_ORDER : u8 {
	NEAR=0,
	LINE=1,
	QUAD=2,
	CUBE=3,
};

/*******************************************************************************
** API for ppost functions ****************************************************/

enum POST_MODE : u8 {
	C    = 1, // concentration
	CF   = 4, // concentration, flux
	CFP  = 7, // concentration, flux, pressure
	CFPS = 10 // concentration, flux, pressure, stress
};

template<u8 nd> using ppost_fn_t
= RET_ERRC
  (const grid_t<nd> &, const pstore_t &, vcache_t<f32> &);

/*******************************************************************************
** API for ppush functions ****************************************************/

enum PUSH_MODE : u8 {
	LEAPF = 0, // expicit  (leaf-frog)
	IMPL0 = 1, // implicit (predictor)
	IMPLR = 2, // implicit (corrector)
	
	EMFLAG = 0b100,  //electromagnetic, 0=electrostatic 
	RLFLAG = 0b1000, //relativistic case
};

//rz-flag
//es-flag

template<u8 nd>
using ppush_fn_t = RET_ERRC (
	const grid_t<nd> &,
	pstore_t &,
	const vcache_t<f32> &,
	f32,
	u32
);

/*******************************************************************************
** API for order functions ***************************************************/

template<u8 nd>
using order_fn_t = RET_ERRC (
	const grid_t<nd> &,
	pstore_t &
);

/*******************************************************************************
** API for pcheck functions ***************************************************/

template<u8 nd>
using pcheck_fn_t = RET_ERRC (
	const grid_t<nd> &,
	const pstore_t &,
	f32
);

/*******************************************************************************
** API for remap/unmap functions **********************************************/

enum REMAP_MODE : u8 {
	NODES = 0, // copy global data into local nodes
	ARRAY = 1, // collect nodes' data into global array
};

template<u8 nd, typename tp> 
using remap_fn_t = void (
	const grid_t<nd> &,
	vcache_t<tp> &,
	tp *
);

/*******************************************************************************
** API for Monte-Carlo simulation functions ***********************************/

template<u8 nd> using mcsim_fn_t = RET_ERRC (
	const grid_t<nd> &,
	pstore_t &,
	vcache_t<u32> &,
	const csection_set_t &,
	const vcache_t<f32> &,
	f32,
	u32
);

/*******************************************************************************
** API for local-simulation ***************************************************/

using mcsim_fn_local_t = RET_ERRC (
	pstore_t &, u32 *, const csection_set_t &, f32 *, f32, u32
);

using ppush_fn_local_t = RET_ERRC (
	pstore_t &, f32*, f32, u32
);

/*******************************************************************************
** API for poisson equation ***************************************************/

enum GRAD_MODE : u8 {
	REGULAR = 0,
	LFBOUND = 1,
	RTBOUND = 2,
	LFLOOP  = 3,
	RTLOOP  = 4,
	LFSAME  = 5,
	RTSAME  = 6,
	SKIP    = 7,
};

template<u8 nd> using poisson_eq_fn_t = f32 (
	const grid_t<nd> &,
	const u8 *,
	std::complex<f32> *,
	f32 *,
	u8,
	u8
);

/******************************************************************************/

template<u8 nd, typename tp>
struct ndarray_iface {
	size_t     shape[nd];
	size_t     offst[nd+1];
	ptrdiff_t  strides[nd];
	tp        *ptr;

	inline
	tp& operator [] (size_t k) const {
		return ptr[k];
	}
};

/******************************************************************************/

#include "api_grid.hxx"
#include "api_pstore.hxx"
#include "api_vcache.hxx"
#include "api_csection_set.hxx"

#endif
