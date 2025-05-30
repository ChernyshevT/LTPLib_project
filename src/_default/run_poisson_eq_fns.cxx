/* #01 ************************************************************************/
extern "C" LIB_EXPORT
f32 SOR_iter1_fn (poisson_eq_t<1> &eq, f32 w) {
	return run_SOR_iter<1>(eq, w);
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
f32 SOR_iter2_fn (poisson_eq_t<2> &eq, f32 w) {
	return run_SOR_iter<2>(eq, w);
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
f32 SOR_iter3_fn (poisson_eq_t<3> &eq, f32 w) {
	return run_SOR_iter<3>(eq, w);
}