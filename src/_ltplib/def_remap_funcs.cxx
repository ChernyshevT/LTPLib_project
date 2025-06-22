#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include "api_grid.hxx"
#include "api_vcache.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
// here was numpy...

using np_array=py::array;

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"

#include "def_grid.hxx"
#include "def_vcache.hxx"

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;
template<int nd, typename tp>
tp* check_array_arg
(const grid_t<nd> & grid, const py::array & arg, u8 order, u8 vsize=1) {
	
	size_t shape[nd+1]; shape[nd] = vsize;
	size_t offst[nd+2]; offst[nd] = vsize; offst[nd+1] = 1;
	for (ssize_t i{nd-1}; i>=0; --i) {
		size_t a = grid.axes[i][0];
		size_t b = grid.axes[i][grid.shape[i]];
		shape[i] = b-a + order;
		offst[i] = shape[i]*offst[i+1];
	}

	auto info = arg.request();
	if (sizeof(tp) != info.itemsize or py::format_descriptor<tp>::format() != info.format) {
		throw py::buffer_error ("types aren't match");
	}
	if (nd+1 != u8(info.ndim)) {
		throw py::buffer_error
		(fmt::format("ndims aren't match: {} != {}",
		nd+1, info.ndim));
	}
	for (size_t i{0}; i<nd+1; ++i) {
		if (shape[i] != size_t(info.shape[i])) {
			throw py::buffer_error
			(fmt::format("shapes aren't match: {} != {}",
			info.shape, std::vector(shape, shape+nd+1)));
		}
		if (info.strides[i] < 0 or info.strides[i] != ssize_t(offst[i+1]*sizeof(tp))) {
			throw py::buffer_error
			(fmt::format("array slices are not allowed: {} {}",
			info.strides, std::vector(offst+1, offst+nd+2)));
		}
	}

	return reinterpret_cast<tp*>(info.ptr);
}


/******************************************************************************/
const char *REMAP_FN {
R"pbdoc(Function binding to transfer (remap) data between cache friendly
segmented data and global array.

Parameters
----------
vcache: _ltpib.vcache

direction: str
  '<' (from array to data) or '>' (from data to array)

vdata: numpy.ndarray

)pbdoc"
};

/******************************************************************************/
void def_remap_funcs (py::module &m) {
	
	m.def("bind_remap_fn",
	[] (vcache_holder& vcache_h, char mode, py::array &iodata) {
		
		return std::visit([&] <u8 nd, typename tp>
		(const grid_t<nd>& grid, vcache_t<tp>& vcache) -> std::function<void(void)> {
			tp* ptr = check_array_arg<nd, tp>(grid, iodata, vcache.order, vcache.vsize);
			
			std::string backend = "default";
			std::string fn_name = fmt::format (
				"remap{}{}_{}_fn",
				nd,
				datatypecode<tp>(),
				mode == '<' ? "NODES" :
				mode == '>' ? "ARRAY" :
				throw std::invalid_argument
				(fmt::format("invalid mode (\"{}\")", mode))
			);
			logger::debug ("bind {}->{} &grid={}, &vcache={}, &vdata={}"
				, backend
				, fn_name
				, (void*)(&grid)
				, (void*)(&vcache)
				, (void*)(ptr)
			);
			
			auto &&fn = libs[backend].get_function<remap_fn_t<nd,tp>>(fn_name);
			return [&, ptr, fn] () mutable {
				return fn (grid, vcache, ptr);
			};
		}, *(vcache_h.gridp), vcache_h);
	}, "vcache"_a, "direction"_a ,"vdata"_a
	, REMAP_FN
	, py::keep_alive<0, 1>() /* keep vcache */
	, py::keep_alive<0, 3>() /* keep vdata */
	);
}
