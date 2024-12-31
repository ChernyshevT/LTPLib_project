/* #01 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_C_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_C_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_C_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CF_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CF_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CF_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFP_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFP_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFP_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #10 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFPS_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #11 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFPS_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #12 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost1_CFPS_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<1, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #13 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_C_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #14 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_C_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #15 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_C_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #16 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CF_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #17 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CF_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #18 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CF_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #19 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFP_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #20 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFP_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #21 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFP_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #22 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFPS_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #23 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFPS_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #24 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost2_CFPS_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<2, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #25 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_C_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #26 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_C_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #27 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_C_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #28 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CF_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #29 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CF_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #30 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CF_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #31 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFP_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #32 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFP_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #33 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFP_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}
/* #34 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFPS_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
}
/* #35 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFPS_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
}
/* #36 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppost3_CFPS_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	return run_ppost<3, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
}