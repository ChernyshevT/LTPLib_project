#ifndef _DEF_GRIDS_HEADER
#define _DEF_GRIDS_HEADER
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include <functional>
#include <memory>
#include <functional>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <fstream>

#include "api_grid.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_memory.hxx"
// here was numpy...
#include "common/loop_over.hxx"

/******************************************************************************/
struct node_cfg {
	std::vector<u32>
		map;
	std::vector<u32>
		lnk;
	u64
		mshift;
};


struct grid_cfg {
	std::vector<u32>
		shape;
	std::vector<u32>
		units;
	std::vector<f32>
		step;
	std::vector<std::vector<u32>>
		axes;
	std::vector<std::vector<f32>>
		edges;
	std::vector<node_cfg>
		nodes;
	std::vector<u8>
		mask;
	std::vector<u32>
		lctr;
	u8
		flags;
	
	grid_cfg (u8 nd, py::dict cfg);
};

/******************************************************************************/

typedef std::variant<
	grid_t<1>,
	grid_t<2>,
	grid_t<3>
> grid_v;

typedef std::vector<f32>
	step_a;  
typedef std::vector<std::vector<u32>>
	axes_a;
typedef std::optional<std::vector<std::vector<u32>>>
	nodes_a;
typedef std::optional<py::array_t<u8>>
	mask_a;

struct grid_holder : grid_v {
	std::unique_ptr<grid_cfg> cfg;

	struct {
		mem_holder base_h;
	} m;

	 grid_holder (u8, step_a, axes_a, nodes_a, mask_a, py::kwargs);
	~grid_holder (void);
};

#endif
