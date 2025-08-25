#pragma once

#include "typedefs.hxx"
#include "api_backend.hxx"
#include <pybind11/pybind11.h>
namespace py = pybind11;

// define base primitives
void def_csections(py::module &);
void def_grids(py::module &);
void def_pstores(py::module &);
void def_vcache(py::module &);

// define base functions:
void def_remap_funcs(py::module &);
void def_ppost_funcs(py::module &);
void def_ppush_funcs(py::module &);
void def_mcsim_funcs(py::module &);

// field-related stuff
void def_poisson_eq(py::module &);

void check_errc (u32 errc);

typedef std::unordered_map<
	const char*,
	std::unique_ptr<void, std::function<void(void*)>>
> mholder_t;
