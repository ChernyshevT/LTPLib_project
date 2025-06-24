#pragma once
#define API_V "API2025-06-24"

#if defined(_WIN32) || defined(_WIN64)
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT __attribute__ ((visibility ("default")))
#endif

#include "typedefs.hxx"

/*******************************************************************************
** RETURN VALUES **************************************************************/
enum ERR_CODE : u32 {
	SUCCESS          = 0x0,
	INVALID_SEQ      = 0x1,
	PPOST_ERR        = 0x2,
	PPUSH_ERR        = 0x4,
	ORDER_ERR        = 0x8,
	MCSIM_ERR        = 0x10,
	PTOVERFLOW       = 0x20,
	PTOUTOFRANGE     = 0x40,
	PTMAXPROBABILITY = 0x80,
	PTMAXENERGY      = 0x100,
};

struct RET_ERRC {
 u32 flags;
};

/******************************************************************************/
template<u8 nd> struct          grid_t;

template<typename tp> struct    vcache_t;

struct                          pstore_t;

struct                          csection_set_t;

/*******************************************************************************
** API for pstore operations **************************************************/

template<u8 nd> using _inject_fn_t = u32
(const grid_t<nd> &, pstore_t &, u64, f32[], u8);

template<u8 nd> using _extract_fn_t = void
(const grid_t<nd> &, const pstore_t &, u64[], f32[]);

template<u8 nd> using _countpp_fn_t = void
(const grid_t<nd> &, pstore_t &, u64[]);

template<u8 nd> using _reset_fn_t = void
(const grid_t<nd> &, pstore_t &);
 
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

/* nibbles to encode pVDF moments */
enum PPOST_ENUM : u64 {
	/* concentration */
	C0  = 0x1,
	/* flux */
	Fx  = 0x2,
	Fy  = 0x3,
	Fz  = 0x4,
	/* pressure & stress */
	Pxx = 0x5,
	Pyy = 0x6,
	Pzz = 0x7,
	Pxy = 0x8,
	Pxz = 0x9,
	Pyz = 0xa,
};

template<u8 nd>
using ppost_fn_t = \
u32 (const grid_t<nd> &, const pstore_t &, vcache_t<f32> &, u64);

/*******************************************************************************
** API for ppush functions ****************************************************/

/* encode scheme */
enum PUSH_MODE : u8 {
	LEAPF = 0, // expicit  (leaf-frog)
	IMPL0 = 1, // implicit (predictor)
	IMPLR = 2, // implicit (corrector)
};

/* nibbles to encode em-field components */
enum EMF_ENUM : u32 {
	Ex   = 0x0,
	Ey   = 0x1,
	Ez   = 0x2,
	Bx   = 0x3,
	By   = 0x4,
	Bz   = 0x5,
};

template<u8 nd>
using ppush_fn_t = \
u32 (const grid_t<nd> &, pstore_t &, const vcache_t<f32> &, f32, u32);

/*******************************************************************************
** API for remap/unmap functions **********************************************/

enum REMAP_MODE : u8 {
	NODES = 0, // copy global data into local nodes
	ARRAY = 1, // collect nodes' data into global array
};

template<u8 nd, typename tp> 
using remap_fn_t = \
void (const grid_t<nd> &, vcache_t<tp> &, tp[]);

/*******************************************************************************
** API for Monte-Carlo simulation functions ***********************************/

template<u8 nd> using mcsim_fn_t = u32 (
	const grid_t<nd> &,
	pstore_t &, /* pstore will be modified */
	vcache_t<u32> &, /* events counter */
	const csection_set_t &,
	const vcache_t<f32> &,
	f32, /* time step */
	u32 /* random seed */
);

/*******************************************************************************
** API for poisson equation ***************************************************/

template<u8 nd>
struct poisson_eq_t;

template<u8 nd>
using SOR_iter_fn_t = f32 (poisson_eq_t<nd> &, f32);

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
