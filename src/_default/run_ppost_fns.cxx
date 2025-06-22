/* #01 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<1, FORM_ORDER::LINE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<1, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<1, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<2, FORM_ORDER::LINE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<2, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<2, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<3, FORM_ORDER::LINE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<3, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid, u64 fcode) {
	u32 flags;

	flags = run_ppost<3, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid, fcode);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}