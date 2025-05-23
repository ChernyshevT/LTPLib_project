/* Low Temperature Plasma Library
 * _ltplib.cxx def_*.?xx  io_*.?xx
 * 
 * Copyright 2024 Timofey Chernyshev
 * <thunarux@protonmail.com, chernyshev.tv@ihed.ras.ru>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

const char *_LTBLIB {
R"pbdoc(_LTBLib: Low Temperature Plasma LIBrary (former Θ-Hall).
A middle-layer tool to construct PiC+MCC (Particles in Cells + Monte Carlo Collisions) codes.

Copyright © 2024 Timofey Chernyshev aka GNU/Hurt
<thunarux@protonmail.com, chernyshev.tv@ihed.ras.ru>
https://www.researchgate.net/profile/Timofey-Chernyshev

«Получаю данные, 18/38.»
)pbdoc"
};

/******************************************************************************/
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wpedantic"
#include <pybind11/pybind11.h>
#pragma GCC diagnostic pop

#include "typedefs.hxx"
#include "api_backend.hxx"
#pragma message ("using " API_V)

#include "io_strings.hxx"
#include "io_dylibs.hxx"

const dylib & dylibs_t::operator [] (std::string key) {
	static std::map<std::string, dylib> cache;
	char                               *descr, *build, *api_v;
	
	if (not cache.contains(key)) {
		cache.emplace(key, dylib("./", key));
		descr = cache.at(key).get_variable<char *>("descr");
		build = cache.at(key).get_variable<char *>("build");
		api_v = cache.at(key).get_variable<char *>("api_v");
		
		if (0 != strcmp(API_V, api_v)) throw py::import_error \
		(fmt::format("Incompatable APIs: {} != {} ({})", API_V, api_v, key));
		
		logger::debug("load \"_{}\" backend: {}, {}, build: {}"
		, key, descr, api_v, build);
	}

	return cache.at(key);
}

dylibs_t libs;
/******************************************************************************/
PYBIND11_MODULE (_ltplib, m) {
	
	namespace py = pybind11;
	
	m.attr("__doc__")     = _LTBLIB;
	m.attr("__version__") = "build: " __DATE__ " " __TIME__;
	
	m.def("load_backend", [&](std::string backend) {
		std::string out;
		out = py::module::import("_ltplib").attr("__file__").cast<std::string>();
		fmt::print("{}\n", out);
		
		libs[backend];
	});

	// define base primitives
	void def_csections(py::module &);
	void def_grids(py::module &);
	void def_pstores(py::module &);
	void def_vcache(py::module &);
	def_csections(m);
	def_grids(m);
	def_pstores(m);
	def_vcache(m);
	
	// define base functions:
	void def_remap_funcs(py::module &);
	// void def_pspawn_funcs(py::module &);
	void def_ppost_funcs(py::module &);
	void def_ppush_funcs(py::module &);
	// void def_order_funcs(py::module &);
	// void def_pcheck_funcs(py::module &);
	// void def_psort_funcs(py::module &);
	void def_mcsim_funcs(py::module &);
	void def_err_codes(py::module &);
	def_remap_funcs(m);
	// [TBD] def_pspawn_funcs(m);
	def_ppost_funcs(m);
	def_ppush_funcs(m);
	// def_order_funcs(m);
	
	//[TBD] def_psort_funcs(m); // required for mcc (maybe)
	def_mcsim_funcs(m); // monte-carlo simulation
	def_err_codes(m);
	
	// define utils module (poisson eq, fft, etc..):
	
	void def_poisson_eq(py::module &);
	def_poisson_eq(m);
	
}
