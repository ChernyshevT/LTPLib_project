/* #01 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_LEAPF_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_LEAPF_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_LEAPF_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPL0_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPL0_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPL0_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPLR_LINE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPLR_QUAD_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush1_IMPLR_CUBE_fn
(const grid_t<1> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<1, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<1>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #10 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_LEAPF_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #11 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_LEAPF_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #12 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_LEAPF_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #13 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPL0_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #14 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPL0_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #15 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPL0_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #16 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPLR_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #17 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPLR_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #18 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2_IMPLR_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #19 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2c_LEAPF_LINE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 1>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #20 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2c_LEAPF_QUAD_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 1>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #21 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush2c_LEAPF_CUBE_fn
(const grid_t<2> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<2, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 1>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<2>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #22 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_LEAPF_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #23 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_LEAPF_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #24 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_LEAPF_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::LEAPF > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::LEAPF >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::LEAPF;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::LEAPF, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #25 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPL0_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #26 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPL0_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #27 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPL0_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPL0 > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPL0 >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPL0;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPL0, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #28 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPLR_LINE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::LINE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #29 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPLR_QUAD_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::QUAD, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #30 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppush3_IMPLR_CUBE_fn
(const grid_t<3> &grid, pstore_t &pstore, const vcache_t<f32> &field, f32 dt, u32 fcode) {
	u32 flags;
	
	if constexpr (PUSH_MODE::IMPLR > PUSH_MODE::IMPL0) {
		if (PUSH_MODE::IMPLR >= pstore.opts.mode) {
			pstore.opts.mode = PUSH_MODE::IMPLR;
		} else {
			return ERR_CODE::INVALID_SEQ;
		}
	}
	
	flags = run_ppush<3, PUSH_MODE::IMPLR, FORM_ORDER::CUBE, 0>
	(grid, pstore, field, dt, fcode);
	if (flags) {
		return ERR_CODE::PPUSH_ERR | flags;
	}
	
	flags = run_order<3>
	(grid, pstore);
	if (flags) {
		return ERR_CODE::ORDER_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}