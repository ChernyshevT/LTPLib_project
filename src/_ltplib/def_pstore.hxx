#ifndef _DEF_POOLS_HEADER
#define _DEF_POOLS_HEADER

#include "typedefs.hxx"
#include "api_pstore.hxx"
#include "def_grid.hxx"

/******************************************************************************/
typedef \
py::array_t<f32, py::array::c_style|py::array::forcecast> \
parts_input_t;

typedef \
py::array_t<f32, py::array::c_style> \
parts_output_t;

typedef \
std::function<size_t(u8, const parts_input_t&)> \
inject_fn_t;

typedef \
std::function<std::tuple<parts_output_t, std::vector<size_t>>(void)> \
extract_fn_t;

typedef \
std::function<size_t(void)>
count_fn_t;

typedef \
std::function<void(void)>
reset_fn_t;

/******************************************************************************/
struct pstore_cfg {
	std::vector<std::string> ptinfo;
	std::vector<f32>         cffts;
	u32                      npools;
	u32                      npmax;
	u8                       nargs;
	u8                       ntype;
	u8                       idxsh;
	
	size_t                   pparts_sz;
	size_t                   pindex_sz;
	size_t                   pflags_sz;
	
	 pstore_cfg (const grid_holder *, std::vector<py::dict>, u32, u8, py::dict);
};

struct pstore_holder : pstore_t {
	const grid_holder * const 
		gridp;
	struct {
		mem_holder
			pparts_h,
			pindex_h,
			pflags_h;
	} m;
	std::unique_ptr<pstore_cfg> cfg;
	
	inject_fn_t  inject_fn;
	extract_fn_t extract_fn;
	count_fn_t   count_fn;
	reset_fn_t   reset_fn;

	 pstore_holder
	 (const grid_holder&, std::vector<py::dict>, u32, u8, py::kwargs);
	
	~pstore_holder
	 (void);

};

#endif
