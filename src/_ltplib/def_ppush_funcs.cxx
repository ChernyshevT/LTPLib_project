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

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_frontend.hxx"

#include "io_strings.hxx"
#include "io_dylibs.hxx"
extern dylibs_t libs;

/******************************************************************************/
std::tuple<u32, std::string>
parse_emf_descr(const char* ptr) {
	constexpr struct {
		const char* key;
		u32 val;
	} table[] = {
		{"Ex", EMF_ENUM::Ex},
		{"Ey", EMF_ENUM::Ey},
		{"Ez", EMF_ENUM::Ez},
		{"Bx", EMF_ENUM::Bx},
		{"By", EMF_ENUM::By},
		{"Bz", EMF_ENUM::Bz},
	};
	constexpr u8 n = sizeof(table) / sizeof(*table);
	bool used[n]{};
	u32 fcode = 0, count = 0;

	while (*ptr) {
		while (std::isspace(*ptr)) {++ptr;};

		bool matched = false;
		for (u8 i = 0; i < n; ++i) {
			auto len = std::strlen(table[i].key);
			if (std::strncmp(ptr, table[i].key, len)) {
				continue;
			}

			if (used[i]) {
				throw std::invalid_argument(std::format("duplicate token: \"{}\"", table[i].key));
			} used[i] = true;

			ptr += len;

			fcode |= table[i].val << (4 * (count + 1));
			++count;
			matched = true;
			break;
		}

		if (':' == *ptr) {
			++ptr;
			while (std::isspace(*ptr)) {++ptr;};
			return { fcode | (count & 0xf), std::string(ptr)};
		}
		if (!matched)
			throw std::invalid_argument(std::format("invalid descr segment: \"{}\"", ptr));
	}

	throw std::invalid_argument(std::format("incomplete descriptor near \"{}\"", ptr));
}

/******************************************************************************/
const char *PPUSH_FN {
R"pbdoc(This function binding is used to calculate collision-less (streaming)
step of particles' ensemble evolution, t->t+dt.

Parameters
----------

pstore: _ltplib.pstore
  Particles' storage to evolve.

descr: str
  String describing the set of emfield-components and imtegration scheme to use.
  Tokens can be separated by spaces or not, i.e. descr = "ExEyBz:[SCHEME]".
  The following schemes are available (see README.md for details):
  - "LEAPF"         --- explicit leap-frog scheme;
  - "IMPL0"+"IMPLR" --- symmetric semi-implicit scheme (predictor+corrector).

emfield: _ltplib.vcache (order>=1, dtype="f32")
  Vaulue cache to read electromagnetic field components.
  The order of emfield defines the type of form-factor to use.

Returns
----------

Function object with 1 argument:
  bind_ppush_fn(...) -> (dt: float) -> None
The function object's call performs the calculation.
Here, dt --- time step.
)pbdoc"};

/******************************************************************************/
void def_ppush_funcs (py::module &m) {

	m.def("bind_ppush_fn", []
	(pstore_holder &pstore, std::string descr, vcache_holder &emfield_h) 
	{
		auto [fcode, scheme] = parse_emf_descr(descr.c_str());
		
		if (emfield_h.cache.vsize < (0xf&fcode)) throw bad_arg
		("emfield.vsize={} < {}", emfield_h.cache.vsize, (0xf&fcode));
		
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<void(f32)> {
			auto &emfield = std::get<vcache_t<f32>>(emfield_h);
			
			std::string backend = "default";
			std::string fn_name = fmt::format("ppush{}{}_{}_{}_fn",
				nd,
				grid.flags & AXIS_FLAG::CYLINDER ? "CYLINDER" : "",
				scheme,
				emfield.order == 1? "LINE" :
				emfield.order == 2? "QUAD" :
				emfield.order == 3? "CUBE" :
				throw std::invalid_argument(fmt::format("emfield.order={}", emfield.order))
			);
			// check argument
			if (scheme != "LEAPF" and pstore.cfg->nargs< 1+(nd+3)*2) throw bad_arg(
				"pstore.nargs = {} <= {}",pstore.cfg->nargs, (nd+3)*2
			);
			
			auto &&fn = libs[backend].get_function<ppush_fn_t<nd>>(fn_name);
			logger::debug\
			("bind {}->{} &grid={}, &pstore={}, &emfield={}, fcode={:#010x}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&emfield), fcode);
			
			return [&, fcode, fn] (f32 dt) -> void {
				check_errc(fn(grid, pstore, emfield, dt, fcode));
			};
		}, *pstore.gridp);
	}, "pstore"_a, "descr"_a, "emfield"_a
	, PPUSH_FN
	, py::keep_alive<0, 1>() /* keep pstore */
	, py::keep_alive<0, 3>() /* keep emfield */
	);
}
