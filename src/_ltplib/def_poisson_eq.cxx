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
#include "common/poisson_eq.hxx"

#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

typedef std::variant<
	poisson_eq_t<1>,
	poisson_eq_t<2>,
	poisson_eq_t<3>
> poisson_eq_v;

typedef std::unordered_map<
	const char*,
	std::unique_ptr<void, std::function<void(void*)>>
> mholder_t;

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
	 poisson_eq_holder (umap_a _umap, step_a _step) {
		
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
			
			eq.offst[nd] = 1;
			for (u8 i{0u}; i<nd; ++i) {
				eq.shape[nd-i-1] = _umap.request().shape[nd-i-1];
				eq.offst[nd-i-1] = eq.offst[nd-i]*eq.shape[nd-i-1];
				eq.dstep[nd-i-1] = 1.0f/_step[nd-i-1]/_step[nd-i-1];
			}
			
			mholder["umap"]  = {malloc(eq.offst[0]*sizeof(u8)),  &free};
			mholder["cdata"] = {malloc(eq.offst[0]*sizeof(f32)), &free};
			mholder["vdata"] = {malloc(eq.offst[0]*sizeof(f32)), &free};
			
			memcpy((void*)mholder.at("umap").get(), _umap.request().ptr, eq.offst[0]*sizeof(u8));
			
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
			auto fn_name = fmt::format("SOR_iter{}_fn", nd);
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
	
	.def(py::init<umap_a, step_a>()
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
	}, "wrelax"_a=1.0f, "perform SOR-iteration, update poisson_eq.vmap")
	
	; /* end class */


	/****************************************************************************/
	enum uTYPE : u8 { // unit type
		NONE  = 0,
		VALUE = SETVALUE,
		
		XLFOPEN = SETAXIS | (LFDIFF<<0),
		XRTOPEN = SETAXIS | (RTDIFF<<0),
		XCENTER = SETAXIS | (CNDIFF<<0),

		YLFOPEN = SETAXIS | (LFDIFF<<2),
		YRTOPEN = SETAXIS | (RTDIFF<<2),
		YCENTER = SETAXIS | (CNDIFF<<2),

		ZLFOPEN = SETAXIS | (LFDIFF<<4),
		ZRTOPEN = SETAXIS | (RTDIFF<<4),
		ZCENTER = SETAXIS | (CNDIFF<<4),
	};
	
	py::enum_<uTYPE> (cls, "uTYPE", py::arithmetic())
	.value("NONE",    uTYPE::NONE)
	.value("VALUE",   uTYPE::VALUE)
	.value("XLFOPEN", uTYPE::XLFOPEN)
	.value("XRTOPEN", uTYPE::XRTOPEN)
	.value("XCENTER", uTYPE::XCENTER)
	.value("YLFOPEN", uTYPE::YLFOPEN)
	.value("YRTOPEN", uTYPE::YRTOPEN)
	.value("YCENTER", uTYPE::YCENTER)
	.value("ZLFOPEN", uTYPE::ZLFOPEN)
	.value("ZRTOPEN", uTYPE::ZRTOPEN)
	.value("ZCENTER", uTYPE::ZCENTER)

	.export_values()
	;
}

