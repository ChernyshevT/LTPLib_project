#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include "api_grid.hxx"
#include "api_vcache.hxx"
// here were forms...
#include "api_pstore.hxx"

#include <functional>
#include <regex>
#include <map>

#include "def_grid.hxx"
#include "def_pstore.hxx"
#include "def_vcache.hxx"
#include "def_csection_set.hxx"

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_frontend.hxx"

#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;
/******************************************************************************/
const char *PMCSIM_FN {
R"pbdoc(This function binding is to perform collisional (diffusive) step of
particles' ensemble evolution, t->t+dt.
Null-collision Monte-Carlo simulation is used uder the hood, see (README.md).

Parameters
----------

pstore: _ltplib.pstore
  Particles' storage to evolve.

events: _ltplib.vcache (order=0, dtype="u32")
  Value cache to write the number of events in each channel.
  Note, there is no automatic clean-up, values will append.
  Use events.reset() to clean-up counters.

cset: _ltplib.csection_set
  Cross-section database.
  
bgrnd: _ltplib.vcache (order=0, dtype="f32")
  Value cache to store background concentrations to interact with.

Returns
----------

Function object with 2 argument:
  bind_ppush_fn(...) -> (dt: float, seed int) -> None
The function object's call performs the calculation.
Here, dt --- time step, seed --- seed-value for random generator.
)pbdoc"};

/******************************************************************************/
void def_mcsim_funcs (py::module &m) {
	
	m.def("bind_mcsim_fn", []
	(pstore_holder &pstore, vcache_holder &cfreq_h, csection_set_holder &cset, vcache_holder &bgrnd_h)
	{
		if (pstore.gridp != cfreq_h.gridp or pstore.gridp != bgrnd_h.gridp)
		throw std::invalid_argument("incompatable grids");
		
		if (pstore.cfg->ptinfo != cset.cfg->ptinfo)
		throw std::invalid_argument("incompatable \"ptinfo\" parameters");
		
		if (cfreq_h.cache.order != 0 or not cfreq_h.cache.dtype.is(py::dtype::of<u32>()))
		throw std::invalid_argument("result should be order=0, u32");
		
		if (bgrnd_h.cache.order != 0 or not bgrnd_h.cache.dtype.is(py::dtype::of<f32>()))
		throw std::invalid_argument("background should be order=0, f32");
		
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<void(f32, u32)> {
			auto &cfreq = std::get<vcache_t<u32>>(cfreq_h);
			auto &bgrnd = std::get<vcache_t<f32>>(bgrnd_h);
			
			std::string backend = "default";
			std::string fn_name = fmt::format("mcsim{}_fn", nd);
			
			logger::debug("bind {}->{} &grid={}, &pstore={}, &cfreq={}, &cset={}, &bgrnd={}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&cfreq), (void*)(&cset), (void*)(&bgrnd) 
			);
			
			auto &&fn = libs[backend].get_function<mcsim_fn_t<nd>>(fn_name);
			return [&, fn] (f32 dt, u32 seed) -> void {
				check_errc(fn(grid, pstore, cfreq, cset, bgrnd, dt, seed));
			};
		}, *pstore.gridp);
	}, "pstore"_a, "events"_a ,"cset"_a, "bgrnd"_a
	, PMCSIM_FN
	, py::keep_alive<0, 1>() /* keep pstore */
	, py::keep_alive<0, 2>() /* keep events */
	, py::keep_alive<0, 3>() /* keep cset */
	, py::keep_alive<0, 4>() /* keep bgrnd */
	);
	
}
