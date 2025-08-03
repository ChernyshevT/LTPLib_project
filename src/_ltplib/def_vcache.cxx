#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
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
#include "api_vcache.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_memory.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;
// here was numpy...

#include "def_vcache.hxx"

/*
 * https://pybind11.readthedocs.io/en/stable/reference.html#_CPPv46buffer
 * static memoryview from_buffer(void *ptr, ssize_t itemsize, const char *format, detail::any_container<ssize_t> shape, detail::any_container<ssize_t> strides, bool readonly = false)
 */
 
struct vcache_ctor {
	
	vcache_holder &self;
	const py::dict  cfg;
	
	template<typename tp, u8 nd>
	void operator () (vcache_t<tp>& vcache, const grid_t<nd> &grid) {
		self.cache.dtype = py::dtype::of<tp>();
		logger::debug("construct vcache{} ({}, &grid{} = {})",
		datatypecode<tp>(), (void*)(&vcache), nd, (void*)(&grid)
		);
		
		self.cache.shape.resize(nd+1);
		self.cache.order     = py::cast<u8>(cfg.attr("get")("order",0));
		self.cache.vsize     = py::cast<u8>(cfg.attr("get")("vsize",1));
		self.cache.shape[nd] = self.cache.vsize;
		
		vcache.vsize = self.cache.vsize;
		vcache.order = self.cache.order;

		size_t nn{vcache.vsize};
		for (int i{0}; i<nd; ++i) {
			self.cache.shape[i] = grid.axes[i][grid.shape[i]]-grid.axes[i][0]+vcache.order;
			size_t mx{0};
			for (u32 j=1; j<grid.shape[i]+1; ++j) {
				mx = std::max(mx, size_t(grid.axes[i][j]-grid.axes[i][j-1]+vcache.order));
			}
			nn *= mx;
		} vcache.blocksize = nn;
		
		self.m.data_h
			.req(&vcache.data, vcache.blocksize*grid.size)
			.alloc();
		
		//~ self.cache.nodes.reserve(grid.size);
		//~ for (auto k{0u}; k<grid.size; ++k) {
			//~ std::vector<py::ssize_t> shape(nd), strides(nd);
			//~ for (auto i{1u}; i<=nd; ++i) {
				//~ shape  [nd-i] = grid[k].shape[nd-i]+self.cache.order;
				//~ strides[nd-i] = i==1? sizeof(tp) : strides[nd-i+1]*shape[nd-i+1];
			//~ }
			//~ self.cache.nodes.push_back(py::memoryview::from_buffer(
				//~ /* ptr    */ vcache[k],
				//~ /*shape   */ std::move(shape),
				//~ /*strides */ std::move(strides),
				//~ /*readonly*/ true
			//~ ));
		//~ }
		
		// [TODO: move into backend!]
		self.reset_fn = [&] () {
			for (u32 k{0}; k<grid.size; ++k) {
				u32 msize{vcache.vsize * (u32)(sizeof(tp))};
				for (u8 i{0u}; i<nd; ++i) {
					auto id = grid.nodes[k].map[i];
					auto a = grid.axes[i][id];
					auto b = grid.axes[i][id+1];
					msize *= b-a + vcache.order;
				}
				memset(vcache[k], 0, msize);
			}
		};
		
		/* create build-in vmap if requested **************************************/
		{
			std::vector<py::ssize_t> shape(nd+1);
			std::vector<py::ssize_t> strides(nd+1);
			u64                      msize{sizeof(tp)*vcache.vsize};
			shape[nd]   = vcache.vsize;
			strides[nd] = sizeof(tp);
			for (auto i{1u}; i<=nd; ++i) {
				auto a = grid.axes[nd-i][grid.shape[nd-i]];
				auto b = grid.axes[nd-i][0];
				msize *= a-b + self.cache.order;
				shape  [nd-i] = a-b + self.cache.order;
				strides[nd-i] = strides[nd-i+1]*shape[nd-i+1];
			}
			self.mem_h["vmap_ptr"]  = {malloc(msize),  &free};
			
			tp *ptr = (tp*)self.mem_h["vmap_ptr"].get();
			self.buffer_h = py::memoryview::from_buffer(
				/* ptr    */ ptr,
				/*shape   */ std::move(shape),
				/*strides */ std::move(strides),
				/*readonly*/ false
			);
			
			std::string backend = "default";
			std::string fn_name = fmt::format("remap{}{}", nd, datatypecode<tp>());

			auto &&fn_in = libs[backend].get_function<remap_fn_t<nd,tp>>(fn_name+"_NODES_fn");
			self.remap_fns[REMAP_MODE::NODES] = [&, ptr, fn = fn_in] () mutable {
				fn(grid, vcache, ptr);
			};

			auto &&fn_out = libs[backend].get_function<remap_fn_t<nd,tp>>(fn_name+"_ARRAY_fn");
			self.remap_fns[REMAP_MODE::ARRAY] = [&, ptr, fn = fn_out] () mutable {
				fn(grid, vcache, ptr);
			};
		}
		
	}
};
 
vcache_holder:: vcache_holder
(const grid_holder& grid, std::string _dtype, u8 _vsize, u8 _order, py::kwargs kwargs)
	: 
	vcache_v {
		_dtype == "f32"? vcache_v{vcache_t<f32>{}} :
		_dtype == "f64"? vcache_v{vcache_t<f64>{}} :
		_dtype == "u32"? vcache_v{vcache_t<u32>{}} :
		throw std::invalid_argument(fmt::format("dtype = \"{}\"", _dtype))
	},
	gridp{&grid}
{
	std::visit(vcache_ctor{*this, py::dict{"vsize"_a=_vsize, "order"_a=_order, **kwargs}}, *this, grid);
	{
		auto sz{f64(m.data_h.get_size())};
		u8 i{0}; 
		const char* lab[] = {"","Ki","Mi","Gi","Ti"};
		while (sz >= 512.0) {
			sz /= 1024.0; ++i;
		}
		logger::debug("create vcache ({}, {:.3f} {}B)", (void*)(this), sz, lab[i]);
	}

};
vcache_holder::~vcache_holder (void) {
	logger::debug("delete vcache ({})", (void*)(this));
}

/******************************************************************************/
const char *VCACHE_DESCR = {
R"pbdoc(Value cache -- node-local storage for lattice-based values.
)pbdoc"};

const char *VCACHE_CTOR = {
R"pbdoc(Creates value cache.

Parameters
----------
grid : existing grid.

dtype : value datatype
  "f32" for 32-bit float values
  "u32" for 32-bit unsigned values (used to count events during MCC simulation).

vsize : the size of value vector (default is 1)

order : form-factor's order (default is 0)
)pbdoc"};

const char *REMAP_DESCR = {
R"pbdoc(transfer (remap) data between nodes and build-in numpy-buffer:
  mode="in"  or "<" copies data from buffer into nodes
  mode="out" or ">" copies data from nodes into buffer
)pbdoc"};

/******************************************************************************/
void def_vcache(py::module &m) {
	
	py::class_<vcache_holder> (m, "vcache", VCACHE_DESCR)
	
	.def(py::init<const grid_holder&, std::string, u8, u8, py::kwargs>
	(), "grid"_a, "dtype"_a, "vsize"_a=1, "order"_a=0
	, VCACHE_CTOR
	, py::keep_alive<1, 2>())

	.def("__getitem__", [] (vcache_holder &self, py::handle index) {
		return self.buffer_h[index];
	})
	
	.def("__setitem__", [] (vcache_holder &self, py::handle index, py::handle val) {
		self.buffer_h[index] = val;
	})

	.def("reset", [] (vcache_holder& self) -> vcache_holder& {
		self.reset_fn();
		return self;
	}, "put 0 to into all nodes, same as self[...] = 0; self.remap(\"in\")")

	.def("remap",
	[] (vcache_holder& self, py::str mode) -> vcache_holder& {
		switch (_hash(mode)) {
			case "<"_hash:
			case "in"_hash:
				self.remap_fns[REMAP_MODE::NODES]();
				return self;
			case ">"_hash:
			case "out"_hash:
				self.remap_fns[REMAP_MODE::ARRAY]();
				return self;
			default:
				throw bad_arg("mode = \"{}\"", py::cast<std::string>(mode));
		}
	}, "mode"_a,
	REMAP_DESCR)

	.def_property_readonly("order",
	[] (const vcache_holder& self) {
		return self.cache.order;
	}, "form-factor's order to use")
	
	.def_property_readonly("shape",
	[] (const vcache_holder& self) {
		return self.cache.shape;
	}, "buffer's shape, same as self[...].shape")
	
	.def_property_readonly("dtype",
	[] (const vcache_holder& self) {
		return self.cache.dtype;
	}, "buffer's dtype, same as self[...].dtype")

	.def_property_readonly("cfg",
	[] (const vcache_holder& self) {
		return py::dict("shape"_a=self.cache.shape, "dtype"_a=self.cache.dtype);
	}, "helper to construct corresponding array: numpy.empty(**self.cfg)")
	
	;// end class
}
