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

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"

#include "io_dylibs.hxx"
extern dylibs_t libs;
/******************************************************************************/
const char *PMCSIM_FN {
R"pbdoc(This binding is for Monte-Carlo simulation on particles' ensemble. 

Parameters
----------

  :param pstore: particles' ensemble to search for collisions (will be modified)
  
  :param result:
  
  :param cset:
  
  :param bgrnd: backgrounds' densities, [flow velocities, temperatures...]

	:returns: bind_mcsim_fn(...) -> fn_object(dt: f32, seed: int) -> RET_ERRC
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
		
		//if ncsect != cfreq_h.cache.shape[nd]
		
		if (cfreq_h.cache.order != 0 or not cfreq_h.cache.dtype.is(py::dtype::of<u32>()))
		throw std::invalid_argument("result should be order=0, u32");
		
		if (bgrnd_h.cache.order != 0 or not bgrnd_h.cache.dtype.is(py::dtype::of<f32>()))
		throw std::invalid_argument("background should be order=0, f32");
		
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<RET_ERRC(f32, u32)> {
			auto &cfreq = std::get<vcache_t<u32>>(cfreq_h);
			auto &bgrnd = std::get<vcache_t<f32>>(bgrnd_h);
			
			std::string backend = "default";
			std::string fn_name = fmt::format("mcsim{}_fn", nd);
			
			logger::debug("bind {}->{} &grid={}, &pstore={}, &cfreq={}, &cset={}, &bgrnd={}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&cfreq), (void*)(&cset), (void*)(&bgrnd) 
			);
			auto &&fn = libs[backend].get_function<mcsim_fn_t<nd>>(fn_name);
			return [&, fn] (f32 dt, u32 seed) {
				auto tv0 = std::chrono::high_resolution_clock::now();
				RET_ERRC retv = fn (grid, pstore, cfreq, cset, bgrnd, dt, seed);
				auto tv1 = std::chrono::high_resolution_clock::now();

				retv.dtime = std::chrono::duration_cast<std::chrono::microseconds>(tv1-tv0).count()*1e-6;
				return retv;
			};
		}, *pstore.gridp);
	}, "pstore"_a, "cfreq"_a ,"cset"_a, "bgrnd"_a, PMCSIM_FN);
	
}
