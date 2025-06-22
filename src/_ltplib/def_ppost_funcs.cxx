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
u64 parse_mode_string(const char* mode_str) {
	constexpr struct {
		const char* key;
		u64 val;
	} table[] = {
		{"C",   PPOST_ENUM::C0},
		{"Fx",  PPOST_ENUM::Fx},
		{"Fy",  PPOST_ENUM::Fy},
		{"Fz",  PPOST_ENUM::Fz},
		{"Pxx", PPOST_ENUM::Pxx},
		{"Pyy", PPOST_ENUM::Pyy},
		{"Pzz", PPOST_ENUM::Pzz},
		{"Pxy", PPOST_ENUM::Pxy},
		{"Pxz", PPOST_ENUM::Pxz},
		{"Pyz", PPOST_ENUM::Pyz}
	};
	constexpr u8 n{sizeof(table)/sizeof(*table)};

	bool used[n] = {};
	u64  fcode = 0;
	u64  count = 0;

	while (*mode_str) {
		if (std::isspace(*mode_str)) {
			mode_str += 1;
			continue;
		}
		bool matched = false;
		for (u8 i{0u}; i<n; ++i) {
			size_t len = std::strlen(table[i].key);
			if (std::strncmp(mode_str, table[i].key, len) == 0) {
				if (used[i]) {
					throw bad_arg("duplicate token: \"{}\"", table[i].key);
				}
				used[i] = true;
				fcode = fcode | (table[i].val << (4*(1+count)));
				count++;
				mode_str += len;
				matched = true;
				break;
			}
		}
		if (!matched) {
			throw bad_arg("invalid descr segment: \"{}\"", mode_str);
		}
	}

	return fcode | count;
}

/******************************************************************************/
const char * PPOST_FN {
R"pbdoc(This function binding is used to post pVDF moments on the grid units.
The 'order' of value cache defines the order of the convolution kernel.

Parameters
----------

pstore: _ltplib.pstore
  Particles' storage to read from

descr: str
  List of pVDF moments to calculate:
  - concentration     "C"
  - flux              "F[x|y|z]"
  - pressure/stress   "P[xx|xy|xz|yy|yz|zz]"
  Tokens can be separated by spaces or not, i.e. descr = "CFx Fy Fz Pxx Pyy Pzz"

ptfluid: _ltplib.vcache
  Value caache to write in.
 
Returns
----------

  Function object. The call performs the calculation and return RET_ERRC object:
  bind_ppost_fn(...) -> fn_object() -> RET_ERRC
)pbdoc"
};

/******************************************************************************/
void def_ppost_funcs (py::module &m) {

	m.def("bind_ppost_fn",
	[] (pstore_holder &pstore, std::string descr, vcache_holder &ptfluid_h) {
		return std::visit([&] <u8 nd>
		(const grid_t<nd>& grid) -> std::function<RET_ERRC(void)> {
			auto &ptfluid = std::get<vcache_t<f32>>(ptfluid_h);
			
			u64 fcode = parse_mode_string(descr.c_str());
			u64 vsize = (fcode & 0xf)*pstore.cfg->ntype;
			
			if (ptfluid.vsize < vsize) throw bad_arg \
			("ptfluid.vsize={} < {}", ptfluid.vsize, vsize);

			std::string backend = "default";
			std::string fn_name = fmt::format("ppost{}_{}_fn",
				nd,
				ptfluid.order == 1? "LINE" :
				ptfluid.order == 2? "QUAD" :
				ptfluid.order == 3? "CUBE" :
				throw bad_arg("ptfluid.order = {}", ptfluid.order)
			);
			
			logger::debug("bind {}->{} &grid={}, &pstore={}, &ptfluid={}, fcode={:#018x}",
			backend, fn_name, (void*)(&grid), (void*)(&pstore), (void*)(&ptfluid), fcode);
			
			auto &&fn = libs[backend].get_function<ppost_fn_t<nd>>(fn_name);
			return [&, fcode, fn] (void) -> RET_ERRC {
				
				return RET_ERRC{fn (grid, pstore, ptfluid, fcode)};
			};
		}, *pstore.gridp);
	}, "pstore"_a, "descr"_a, "ptfluid"_a
	, PPOST_FN
	, py::keep_alive<0, 1>() /* keep pstore */
	, py::keep_alive<0, 3>() /* keep ptfluid */
	);
}


