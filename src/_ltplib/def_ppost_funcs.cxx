#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <functional>
#include <chrono>
namespace py = pybind11;
using namespace pybind11::literals;

#include "api_grid.hxx"
#include "api_vcache.hxx"
// here were forms...
#include "api_pstore.hxx"

#include "def_grid.hxx"
#include "def_pstore.hxx"
#include "def_vcache.hxx"

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

/******************************************************************************/
const char * PPOST_FN {
R"pbdoc(This function binding is used to obtain pVDF moments on the grid cells.
Segmented array's order defines the order of the convolution kernel.

Parameters
----------

pstore : particles' storage to read from

ptfluid : segmented array to write in

mode : list of pVDF moments to calculate:
	[*] "C"    concentration: {n}
	[*] "CF"   concentration, flux: {n,fx,fy,fz}
	[*] "CFP"  concentration, flux, pressure: {n,fx,fy,fz,pxx,pyy,pzz}
	[*] "CFPS" concentration, flux, pressure+stress: {n,fx,fy,fz,pxx,pyy,pzz,pxt,pxz,pyz}"

Returns
----------

	Function object. The call performs the calculation and return RET_ERRC object:
	bind_ppost_fn(...) -> fn_object() -> RET_ERRC
)pbdoc"
};

/******************************************************************************/
void def_ppost_funcs (py::module &m) {

	m.def("bind_ppost_fn",
	[] (pstore_holder &pstore, vcache_holder &ptfluid_h, py::str mode) {
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<RET_ERRC(void)> {
			auto &ptfluid = std::get<vcache_t<f32>>(ptfluid_h);
			
			// check input argument
			u8 vsize{pstore.cfg->ntype};
			switch (_hash(mode)) {
				case "C"_hash:
					vsize *= POST_MODE::C;
					break;
				case "CF"_hash:
					vsize *= POST_MODE::CF;
					break;
				case "CFP"_hash:
					vsize *= POST_MODE::CFP;
					break;
				case "CFPS"_hash:
					vsize *= POST_MODE::CFPS;
					break;
				default: throw std::invalid_argument \
				(fmt::format("mode= \"{}\"", (std::string)(mode)));
			}
			if (ptfluid.vsize >= vsize) {
				vsize = ptfluid.vsize;
			} else throw std::invalid_argument \
			(fmt::format("vsize = {} < {}", ptfluid.vsize, vsize));

			std::string backend = "default";
			std::string fn_name = fmt::format("ppost{}_{}_{}_fn",
				nd,
				(std::string)(mode),
				ptfluid.order == 1? "LINE" :
				ptfluid.order == 2? "QUAD" :
				ptfluid.order == 3? "CUBE" :
				throw std::invalid_argument("ptfluid.order")
			);
			
			logger::debug("bind {}->{} &grid={}, &pstore={}, &ptfluid={}, mode={}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&ptfluid), (std::string)(mode));
			
			auto &&fn = libs[backend].get_function<ppost_fn_t<nd>>(fn_name);
			return [&, fn] (void) -> RET_ERRC {
				return RET_ERRC{fn (grid, pstore, ptfluid)};
			};
		}, *pstore.gridp);
	}, "pstore"_a, "ptfluid"_a, "mode"_a="C"
	, PPOST_FN
	, py::keep_alive<0, 1>()
	, py::keep_alive<0, 2>()
	);
}


