#ifndef _DEF_LATTICES_HEADER
#define _DEF_LATTICES_HEADER
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

#include "io_memory.hxx"
#include "api_vcache.hxx"
#include "api_backend.hxx"
#include "def_grid.hxx"

struct vcache_cfg {
	u32                      order;
	u32                      vsize;
	std::vector<u64>         shift;
	py::dtype                dtype;
	std::vector<py::ssize_t> shape;
	
	vcache_cfg (const grid_holder&, py::dict);
};

typedef std::variant<
	vcache_t<f32>,
	vcache_t<f64>,
	vcache_t<u32>
> vcache_v;

struct vcache_holder : vcache_v {
	struct {
		mem_holder data_h;
	} m;
	const grid_holder * const  gridp;
	struct {
		std::vector<py::ssize_t> shape;
		py::dtype                dtype;
		u8                       order;
		u8                       vsize;
		std::vector<py::array>   nodes;
	} cache;
	
	std::function<void(void)>  reset_fn;
	
	/* vcache_holder (const grid_holder&, std::string, py::dict);*/
	 vcache_holder
	 (const grid_holder&, std::string, u8, u8, py::kwargs);
	
	~vcache_holder
	 (void);

};

#endif
