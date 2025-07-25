/* #01 ************************************************************************/
extern "C" LIB_EXPORT
u32 mcsim1_fn (
const grid_t<1> &grid, pstore_t &pstore, vcache_t<u32> &events, const csection_set_t &cset, const vcache_t<f32> &bgrnd, f32 dt, u32 seed) {
	u32 flags;
	flags = run_mcsim<1> (grid, pstore, events, cset, bgrnd, dt, seed);
	if (flags) {
		return ERR_CODE::MCSIM_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
u32 mcsim2_fn (
const grid_t<2> &grid, pstore_t &pstore, vcache_t<u32> &events, const csection_set_t &cset, const vcache_t<f32> &bgrnd, f32 dt, u32 seed) {
	u32 flags;
	flags = run_mcsim<2> (grid, pstore, events, cset, bgrnd, dt, seed);
	if (flags) {
		return ERR_CODE::MCSIM_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
u32 mcsim3_fn (
const grid_t<3> &grid, pstore_t &pstore, vcache_t<u32> &events, const csection_set_t &cset, const vcache_t<f32> &bgrnd, f32 dt, u32 seed) {
	u32 flags;
	flags = run_mcsim<3> (grid, pstore, events, cset, bgrnd, dt, seed);
	if (flags) {
		return ERR_CODE::MCSIM_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}