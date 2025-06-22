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
#include <map>
#include <functional>
#include <regex>
#include <chrono>

#include "def_grid.hxx"
#include "def_pstore.hxx"
#include "def_vcache.hxx"

#include "api_backend.hxx"
#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

/******************************************************************************/
const char *PPUSH_FN {
R"pbdoc(This function binding is used to calculate collision-less (streaming)
step of particles' ensemble evolution, t->t+dt.

Note, particle form-factor's order defined by order of emfield.

Returns
----------

	Function object with 2 arguments.
	The function object's call performs the calculation
	and return RET_ERRC object:
	bind_ppush_fn(...) -> fn_object(dt) -> RET_ERRC, where dt is the time-step.

Parameters
----------
	pstore: particles' storage to evolve. Note that after the streaming step the
	integrity of the storage is violated. Additional step (see _ltplib.order_fn)
	is required to fix the integrity.
	
	emfield:
		Segmented array containing electromagnetic field vectors.
	
	descr:
		String to describe the order of components in emfield-vector,
		and type of the integrator scheme ("LEAPF" if omitted),
		for example descr = "Ex Ey Bz : MODE".

)pbdoc"};

/******************************************************************************/
void def_ppush_funcs (py::module &m) {

	m.def("bind_ppush_fn", []
	(pstore_holder &pstore, std::string descr, vcache_holder &emfield_h) 
	{
		
		std::string mode = "LEAPF";
		u32 fcode{0}, n{0};
		for (const char *ptr = &descr[0]; *ptr != '\0'; ++ptr) {
			if (isspace(*ptr)) {
				continue;
			}
			if (*ptr == ':') {
				++ptr; while (isspace(*ptr)) ++ptr;
				
				mode = std::move(std::string(ptr));
				if (mode.empty()) throw \
				std::invalid_argument(ptr);
				
				break;
			}
			if (ptr[1] == '\0' or ptr[1] == ':' or isspace(ptr[1])) {
				throw std::invalid_argument(ptr);
			}
			switch (ptr[0]) {
				case 'E':
					switch (ptr[1]) {
						case 'x':
							fcode += 0x8 << (4 * n++); break;
						case 'y':
							fcode += 0x9 << (4 * n++); break;
						case 'z':
							fcode += 0xA << (4 * n++); break;
						default:
							throw std::invalid_argument(ptr);
					}
					break;
				case 'B':
					switch (ptr[1]) {
						case 'x':
							fcode += 0xB << (4 * n++); break;
						case 'y':
							fcode += 0xC << (4 * n++); break;
						case 'z':
							fcode += 0xD << (4 * n++); break;
						default:
							throw std::invalid_argument(ptr);
					}
					break;
				default:
					throw std::invalid_argument(ptr);
			}
			++ptr;
		}
		if (emfield_h.cache.vsize < n) throw std::invalid_argument(fmt::format\
		("emfield.vsize={} < {} it isn't big enough to represent all the components",
		emfield_h.cache.vsize, n));
		
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<RET_ERRC(f32)> {
			auto &emfield = std::get<vcache_t<f32>>(emfield_h);
			
			std::string backend = "default";
			std::string fn_name = fmt::format("ppush{}{}_{}_{}_fn",
				nd,
				grid.flags.cylcrd? "c" : "",
				mode,
				emfield.order == 1? "LINE" :
				emfield.order == 2? "QUAD" :
				emfield.order == 3? "CUBE" :
				throw std::invalid_argument(fmt::format("emfield.order={}", emfield.order))
			);
			// check argument
			if (mode != "LEAPF" and pstore.cfg->nargs< 1+(nd+3)*2) throw bad_arg(
				"pstore.nargs = {} <= {}",pstore.cfg->nargs, (nd+3)*2
			);
			
			logger::debug\
			("bind {}->{} &grid={}, &pstore={}, &emfield={}, fcode={}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&emfield), fcode);
			
			auto &&fn = libs[backend].get_function<ppush_fn_t<nd>>(fn_name);
			
			return [&, fcode, fn] (f32 dt) -> RET_ERRC {
				return RET_ERRC{fn (grid, pstore, emfield, dt, fcode)};
			};
		}, *pstore.gridp);
	}, "pstore"_a, "descr"_a, "emfield"_a
	, PPUSH_FN
	, py::keep_alive<0, 1>() /* keep pstore */
	, py::keep_alive<0, 3>() /* keep emfield */
	);
}
