/* Low Temperature Plasma Library
 * _ltplib.cxx def_*.?xx  io_*.?xx
 * 
 * Copyright 2025 Timofey Chernyshev
 * <thunarux@protonmail.com, chernyshev.tv@ihed.ras.ru>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

const char *_LTBLIB_DOC {
R"pbdoc(_ltplib: Low Temperature Plasma LIBrary (former Θ-Hall).

A middle-layer tool to construct PiC+MCC (Particles in Cells + Monte Carlo Collisions) codes. [https://github.com/ChernyshevT/LTPSim_project]

Copyright © 2025 Timofey Chernyshev aka GNU/Hurt
<thunarux@protonmail.com, chernyshev.tv@ihed.ras.ru>
[https://www.researchgate.net/profile/Timofey-Chernyshev]

«Получаю данные, 18/38.»
)pbdoc"};

/******************************************************************************/

#include "typedefs.hxx"
#include "api_backend.hxx"
#include "api_frontend.hxx"
#pragma message ("using " API_V)

#include <filesystem>
#include "io_strings.hxx"
#include "io_dylibs.hxx"

dylibs_t libs;
/******************************************************************************/
PYBIND11_MODULE (_ltplib, m) {
	
	namespace py = pybind11;
	
	m.attr("__doc__")     = _LTBLIB_DOC;
	m.attr("__version__") = "v0.1, build: " __DATE__ " " __TIME__;
	m.attr("__license__") = "GNU LGPL-3.0";
	
	m.def("load_backend", [&](std::string backend) {
		libs[backend];
	});

	def_csections(m);
	def_grids(m);
	def_pstores(m);
	def_vcache(m);
	
	//def_remap_funcs(m);
	def_ppost_funcs(m);
	def_ppush_funcs(m);
	def_mcsim_funcs(m);
	
	def_poisson_eq(m);
}

/******************************************************************************/
void check_errc (u32 errc) {
	std::string msg;
	
	if (not errc) [[likely]] {
		return;
	}
	
	if (errc & ERR_CODE::INVALID_SEQ) {
		msg = std::format("{}{}INVALID_SEQ",      msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PPOST_ERR) {
		msg = std::format("{}{}PPOST",            msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PPUSH_ERR) {
		msg = std::format("{}{}PPUSH",            msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::ORDER_ERR) {
		msg = std::format("{}{}ORDER",            msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::MCSIM_ERR) {
		msg = std::format("{}{}MCSIM",            msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PTOVERFLOW) {
		msg = std::format("{}{}PTOVERFLOW",       msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PTOUTOFRANGE) {
		msg = std::format("{}{}PTOUTOFRANGE",     msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PTMAXENERGY) {
		msg = std::format("{}{}PTMAXENERGY",      msg, !msg.empty()? "|": "");
	}
	if (errc & ERR_CODE::PTMAXPROBABILITY) {
		msg = std::format("{}{}PTMAXPROBABILITY", msg, !msg.empty()? "|": "");
	}
	throw std::runtime_error(msg);
}

/******************************************************************************/
const dylib & dylibs_t::operator [] (std::string key) {
	namespace fs = std::filesystem;
	
	static std::map<std::string, dylib> cache;
	char                               *descr, *build, *api_v;
	
	auto path = \
	std::filesystem::path(py::module::import("_ltplib")
	.attr("__file__")
	.cast<std::string>())
	.parent_path();
	
	if (not cache.contains(key)) try {
		cache.emplace(key, dylib(path, key));
		descr = cache.at(key).get_variable<char *>("descr");
		build = cache.at(key).get_variable<char *>("build");
		api_v = cache.at(key).get_variable<char *>("api_v");

		logger::info("using \"_{}\" backend: {}, {}, build: {}"
		, key, descr, api_v, build);

		if (0 != strcmp(API_V, api_v)) {
			throw bad_import("Incompatable APIs: {} != {} ({})", API_V, api_v, key);
		}
	} catch (std::exception &e) {
		throw bad_import("Failed to load backend: {}!", e.what());
	}

	return cache.at(key);
}
