/* #01 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_LEAPF_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_LEAPF_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_LEAPF_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPL0_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPL0_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPL0_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPLR_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPLR_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush1_IMPLR_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #10 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_LEAPF_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #11 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_LEAPF_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #12 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_LEAPF_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #13 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPL0_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #14 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPL0_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #15 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPL0_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #16 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPLR_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #17 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPLR_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #18 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2_IMPLR_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #19 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2c_LEAPF_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 1>
	(grid, pstore, field, dt, fcode);
}
/* #20 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2c_LEAPF_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 1>
	(grid, pstore, field, dt, fcode);
}
/* #21 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush2c_LEAPF_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 1>
	(grid, pstore, field, dt, fcode);
}
/* #22 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_LEAPF_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #23 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_LEAPF_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #24 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_LEAPF_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #25 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPL0_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #26 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPL0_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #27 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPL0_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #28 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPLR_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
}
/* #29 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPLR_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
}
/* #30 ************************************************************************/
extern "C" LIB_EXPORT
RET_ERRC ppush3_IMPLR_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	return run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
}