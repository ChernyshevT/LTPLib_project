/* #01 ************************************************************************/
extern "C" LIB_EXPORT
void remap1f32_NODES_fn
(const grid_t<1> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_nodes<1, f32>
	(grid, latt, g_data);
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
void remap1u32_NODES_fn
(const grid_t<1> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_nodes<1, u32>
	(grid, latt, g_data);
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
void remap1f32_ARRAY_fn
(const grid_t<1> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_array<1, f32>
	(grid, latt, g_data);
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
void remap1u32_ARRAY_fn
(const grid_t<1> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_array<1, u32>
	(grid, latt, g_data);
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
void remap2f32_NODES_fn
(const grid_t<2> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_nodes<2, f32>
	(grid, latt, g_data);
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
void remap2u32_NODES_fn
(const grid_t<2> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_nodes<2, u32>
	(grid, latt, g_data);
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
void remap2f32_ARRAY_fn
(const grid_t<2> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_array<2, f32>
	(grid, latt, g_data);
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
void remap2u32_ARRAY_fn
(const grid_t<2> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_array<2, u32>
	(grid, latt, g_data);
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
void remap3f32_NODES_fn
(const grid_t<3> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_nodes<3, f32>
	(grid, latt, g_data);
}
/* #10 ************************************************************************/
extern "C" LIB_EXPORT
void remap3u32_NODES_fn
(const grid_t<3> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_nodes<3, u32>
	(grid, latt, g_data);
}
/* #11 ************************************************************************/
extern "C" LIB_EXPORT
void remap3f32_ARRAY_fn
(const grid_t<3> &grid, vcache_t<f32> &latt, f32 *g_data ) {
	run_remap_to_array<3, f32>
	(grid, latt, g_data);
}
/* #12 ************************************************************************/
extern "C" LIB_EXPORT
void remap3u32_ARRAY_fn
(const grid_t<3> &grid, vcache_t<u32> &latt, u32 *g_data ) {
	run_remap_to_array<3, u32>
	(grid, latt, g_data);
}