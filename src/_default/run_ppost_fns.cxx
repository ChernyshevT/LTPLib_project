/* #01 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_C_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #02 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_C_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #03 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_C_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #04 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CF_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #05 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CF_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #06 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CF_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #07 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFP_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #08 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFP_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #09 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFP_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #10 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFPS_LINE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #11 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFPS_QUAD_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #12 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost1_CFPS_CUBE_fn
(const grid_t<1> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<1, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #13 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_C_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #14 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_C_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #15 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_C_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #16 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CF_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #17 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CF_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #18 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CF_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #19 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFP_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #20 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFP_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #21 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFP_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #22 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFPS_LINE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #23 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFPS_QUAD_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #24 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost2_CFPS_CUBE_fn
(const grid_t<2> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<2, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #25 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_C_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::C, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #26 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_C_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::C, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #27 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_C_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::C, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #28 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CF_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CF, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #29 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CF_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CF, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #30 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CF_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CF, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #31 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFP_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFP, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #32 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFP_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFP, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #33 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFP_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFP, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #34 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFPS_LINE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFPS, FORM_ORDER::LINE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #35 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFPS_QUAD_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFPS, FORM_ORDER::QUAD>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}
/* #36 ************************************************************************/
extern "C" LIB_EXPORT
u32 ppost3_CFPS_CUBE_fn
(const grid_t<3> &grid, const pstore_t &pstore, vcache_t<f32> &ptfluid) {
	u32 flags;

	flags = run_ppost<3, POST_MODE::CFPS, FORM_ORDER::CUBE>
	(grid, pstore, ptfluid);
	if (flags) {
		return ERR_CODE::PPOST_ERR | flags;
	} else {
		return ERR_CODE::SUCCESS;
	}
}