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

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

const char *PORDER_FN {
R"pbdoc(Restore particle storage integrity.
It is necessary to use this binding after ppush-step.
)pbdoc"
};
/******************************************************************************/
void def_order_funcs (py::module &m) {

	m.def("bind_order_fn", []
	(pstore_holder &pstore) 
	{
		if (not pstore.gridp) throw std::logic_error (
		"Can not use this binding for spatial-local mode!");
		
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<RET_ERRC(void)> {
			
			auto backend = "default";
			auto fn_name = fmt::format("order{}_fn", nd);
			
			logger::debug("bind {}->{} &grid={}, &pstore={}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore));
			
			auto fn = libs[backend].get_function<order_fn_t<nd>>(fn_name);
			return [&, fn] (void) {
				auto tv0 = std::chrono::high_resolution_clock::now();
				RET_ERRC retv = fn (grid, pstore);
				auto tv1 = std::chrono::high_resolution_clock::now();

				retv.dtime = std::chrono::duration_cast<std::chrono::microseconds>(tv1-tv0).count()*1e-6;
				return retv;
			};
		}, *pstore.gridp);
	}, "pstore"_a,
	PORDER_FN);

}
