#pragma once

//@LISTING{start:loop_over_form}
template<u8 len, u8 nd, u8 md=0, typename func>
void loop_over_form
(func&& fn, size_t *offset, u32 *idx, f32 *form, f32 w=1.0f, size_t k=0) {
	if constexpr (md<nd) for (u8 i{0}; i<len; ++i) {
		loop_over_form <len, nd, md+1>
		(fn, offset, idx, form, w*form[md*len+i], k+(idx[md]+i)*offset[md+1]);
	} else {
		fn(w, k);
	}
}
//@LISTING{end:loop_over_form}

//@LISTING{start:loop_over_block}
template<u8 nd, u8 md=0, typename func>
void loop_over_block
(func&& fn, size_t *g_off, size_t *g_pos, size_t *l_off, size_t *l_pos, size_t gk=0, size_t k=0) {
	if constexpr (md<nd) for (size_t i=l_pos[md*2]; i<l_pos[md*2+1]; ++i) {
		loop_over_block<nd, md+1>
		(fn, g_off, g_pos, l_off, l_pos, gk + g_off[md+1]*(i+g_pos[md]), k + l_off[md+1]*i);
	} else {
		fn(gk, k);
	}
}
//@LISTING{end:loop_over_block}

namespace {
	template<u8 nd, u8 md=0, typename func>
	void loop_over_edges
	(func&& fn, u8 ax, size_t ng, size_t *offst, size_t* shape, size_t *idx) {

		if constexpr (md<nd) {
			for (size_t i{0}, n{(ax != md) ? shape[md] : 1}; i<n; ++i) {
				idx[md] = i;
				loop_over_edges<nd, md+1>(fn, ax, ng, offst, shape, idx);
			}
		} else for (size_t k{0}; k<ng; ++k) {
			size_t j0{0}, j1{0};
			for (size_t i{0}; i<nd; ++i) {
				if (i != ax) {
					j0 += offst[i+1]*idx[i];
					j1 += offst[i+1]*idx[i];
				} else {
					j0 += offst[i+1]*k;
					j1 += offst[i+1]*(shape[i]+k-ng);
				}
			}
			for (size_t i{0}; i<offst[nd]; ++i) {
				fn(j0, j1, i);
			}
		}
	}
}

template<u8 nd, typename func>
void loop_over_bound
(func&& fn, u8 ax, size_t ng, size_t *offst, size_t *shape) {
	size_t idx[nd];
	loop_over_edges<nd>(fn, ax, ng, offst, shape, idx);
}
