#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include <vector>
#include <iostream>
#include <complex.h>

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_frontend.hxx"
#include "common/poisson_eq.hxx"

#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

typedef std::variant<
	poisson_eq_t<1>,
	poisson_eq_t<2>,
	poisson_eq_t<3>
> poisson_eq_v;

typedef const py::array_t<u8, py::array::c_style>&
  umap_a;

typedef const std::vector<f32> &
  step_a;

typedef std::function<f32(f32)>
  iter_fn_t;

/******************************************************************************/
struct poisson_eq_holder {
	
	poisson_eq_v eq;
	mholder_t    mholder;
	
	py::array umap;
	py::array cmap;
	py::array vmap;
	iter_fn_t iter_fn;
	
	/* ctor */
	 poisson_eq_holder (umap_a _umap, step_a _step, py::kwargs kw) {
		
		switch (_umap.request().ndim) {
			case 1:
				this->eq = poisson_eq_v{poisson_eq_t<1>{}};
				break;
			case 2:
				this->eq = poisson_eq_v{poisson_eq_t<2>{}};
				break;
			case 3:
				this->eq = poisson_eq_v{poisson_eq_t<3>{}};
				break;
			default:
				throw bad_arg("invalid range ({})", _umap.request().ndim);
		}
		
		std::visit([&] <u8 nd> (poisson_eq_t<nd> &eq) {
			logger::debug("construct poisson_eq{} ({})", nd, (void*)(&eq));
			
			u64 usize{1};
			
			for (u8 i{1u}; i<=nd; ++i) {
				eq.shape[nd-i] = _umap.request().shape[nd-i];
				eq.dstep[nd-i] = _step[nd-i];
				usize = usize * eq.shape[nd-i];
			}
			
			if (kw.contains("radius")) {
				if (nd != 2) {
					throw bad_arg("radius is valid for 2d problems only!");
				}
				eq.dstep[nd] = py::cast<f32>(kw["radius"]);
				//py::print(kw, eq.dstep[nd]);
			} else {
				eq.dstep[nd] = NAN;
			}
			
			mholder["umap"]  = {malloc(usize*sizeof(u8)),  &free};
			mholder["cdata"] = {malloc(usize*sizeof(f32)), &free};
			mholder["vdata"] = {malloc(usize*sizeof(f32)), &free};
			
			memcpy((void*)mholder.at("umap").get(), _umap.request().ptr, usize*sizeof(u8));
			
			eq.umap  = (u8*)mholder.at("umap").get();
			eq.cdata = (f32*)mholder.at("cdata").get();
			eq.vdata = (f32*)mholder.at("vdata").get();
			
		}, this->eq);

		/**************************************************************************/
		this->umap = 
		std::visit([&] <u8 nd> (poisson_eq_t<nd> &eq) -> py::array_t<u8> {
			std::vector<py::ssize_t> shape(nd), strides(nd);
			for (auto i{1u}; i<=nd; ++i) {
				shape  [nd-i] = eq.shape[nd-i];
				strides[nd-i] = (i==1? sizeof(u8) : strides[nd-i+1]*shape[nd-i+1]);
			}
			return py::memoryview::from_buffer(
				/* ptr    */ eq.umap,
				/*shape   */ std::move(shape),
				/*strides */ std::move(strides),
				/*readonly*/ true
			);
		}, this->eq);
		
		/**************************************************************************/
		this->cmap = 
		std::visit([&] <u8 nd> (poisson_eq_t<nd> &eq) -> py::array_t<f32> {
			std::vector<py::ssize_t> shape(nd), strides(nd);
			for (auto i{1u}; i<=nd; ++i) {
				shape  [nd-i] = eq.shape[nd-i];
				strides[nd-i] = (i==1? sizeof(f32) : strides[nd-i+1]*shape[nd-i+1]);
			}
			return py::memoryview::from_buffer(
				/* ptr    */ eq.cdata,
				/*shape   */ std::move(shape),
				/*strides */ std::move(strides),
				/*readonly*/ false
			);
		}, this->eq);
		
		/**************************************************************************/
		this->vmap = 
		std::visit([&] <u8 nd> (poisson_eq_t<nd> &eq) -> py::array_t<f32> {
			std::vector<py::ssize_t> shape(nd), strides(nd);
			for (auto i{1u}; i<=nd; ++i) {
				shape  [nd-i] = eq.shape[nd-i];
				strides[nd-i] = (i==1? sizeof(f32) : strides[nd-i+1]*shape[nd-i+1]);
			}
			return py::memoryview::from_buffer(
				/* ptr    */ eq.vdata,
				/*shape   */ std::move(shape),
				/*strides */ std::move(strides),
				/*readonly*/ false
			);
		}, this->eq);

		/**************************************************************************/
		this->iter_fn = \
		std::visit([&] <u8 nd> (poisson_eq_t<nd> &eq) -> iter_fn_t {
			
			auto backend = "default";
			auto fn_name = std::format("SOR_iter{}_fn", nd);
			logger::debug ("bind {}->{}", backend, fn_name);
			auto &&fn = libs[backend].get_function<SOR_iter_fn_t<nd>>(fn_name);
			return [&, fn] (f32 w) -> f32 {
				return fn(eq, w);
			};
		}, this->eq);  
	}
	/* end ctor */
	
};

/******************************************************************************/
void def_poisson_eq(py::module &m) {

	py::class_<poisson_eq_holder> cls (m, "poisson_eq"); cls
	
	.def(py::init<umap_a, step_a, py::kwargs>()
	, "universal poisson eq. solver")

	.def_readonly("umap", &poisson_eq_holder::umap,
	"unit-map (readonly)")

	.def_readonly("cmap", &poisson_eq_holder::cmap,
	"charge-map (input)")
	
	.def_readonly("vmap", &poisson_eq_holder::vmap,
	"voltage-map (output)")
	
	.def("iter", [] (poisson_eq_holder& self, f32 w) -> f32 {
		if (w<=0.0f or w>=2.0f) {
			throw bad_arg("invalid wrelax parameter ({})!", w);
		}
		return self.iter_fn(w);
	}, "wrelax"_a=1.0f
	, "performs SOR-iteration, updates poisson_eq.vmap")
	
	.def("solve", []
	(poisson_eq_holder& self, f32 w, f32 err_max) -> std::tuple<f32, u32> {
		if (w<=0.0f or w>=2.0f) {
			throw bad_arg("invalid wrelax parameter ({})!", w);
		}
		if (err_max<=0.0f) {
			throw bad_arg("invalid err_max parameter ({})!", err_max);
		}
		f32 err;
		f32 j{0}; do {
			err = self.iter_fn(w); ++j;
		} while (err > err_max);
		return {err, j};
	}, "wrelax"_a=1.0f, "err_max"_a=0.0f
	, "perform SOR-iteratios till err <= err_max")
	
	; /* end class */


	/****************************************************************************/
	m.def("DIFFop", [] (const char *mode_str) -> u8 {
		constexpr struct {
			const char* key;
			u8          val;
		} table[] = {
			{"NIL", 0},
			{"XLF", LFDIFF<<0},
			{"XRT", RTDIFF<<0},
			{"XCN", CNDIFF<<0},
			{"YLF", LFDIFF<<2},
			{"YRT", RTDIFF<<2},
			{"YCN", CNDIFF<<2},
			{"ZLF", LFDIFF<<4},
			{"ZRT", RTDIFF<<4},
			{"ZCN", CNDIFF<<4},
		};
		constexpr u8 n{sizeof(table)/sizeof(*table)};

		bool matched, used[n]{false};
		u8 flags = 0;
		
		while (*mode_str) {
			if ('|' == *mode_str) {
				mode_str += 1;
				continue;
			}
			matched = false;
			for (u8 i{0u}; i<n; ++i) {
				size_t len = std::strlen(table[i].key);
				if (std::strncmp(mode_str, table[i].key, len) == 0) {
					if (used[i]) {
						throw bad_arg("flag duplication: \"{}\"", table[i].key);
					}
					used[i] = true;
					matched = true;
					flags = flags | table[i].val;
					mode_str += len;
					break;
				}
			}
			if (not matched) {
				throw bad_arg("invalid flag segment: \"{}\"", mode_str);
			}
		}

		return flags;
	});

}

