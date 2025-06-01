#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include <functional>
#include <memory>
#include <functional>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <fstream>

#include "typedefs.hxx"
#include "io_strings.hxx"
#include "io_memory.hxx"

#include "api_backend.hxx"

std::string __repr__(const RET_ERRC & e) {
	std::string msg;
	
	if (not e.flags) {
		return "";
	}
	if (e.flags & ERR_CODE::INVALID_SEQ) {
		msg = fmt::format("{}{}INVALID_SEQ"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PPOST_ERR) {
		msg = fmt::format("{}{}PPOST"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PPUSH_ERR) {
		msg = fmt::format("{}{}PPUSH"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::ORDER_ERR) {
		msg = fmt::format("{}{}ORDER"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::MCSIM_ERR) {
		msg = fmt::format("{}{}MCSIM"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PTOVERFLOW) {
		msg = fmt::format("{}{}PTOVERFLOW"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PTOUTOFRANGE) {
		msg = fmt::format("{}{}PTOUTOFRANGE"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PTMAXENERGY) {
		msg = fmt::format("{}{}PTMAXENERGY"
		, msg, !msg.empty()?"|":"");
	}
	if (e.flags & ERR_CODE::PTMAXPROBABILITY) {
		msg = fmt::format("{}{}PTMAXPROBABILITY"
		, msg, !msg.empty()?"|":"");
	}
	return msg;
}

void def_err_codes(py::module &m) {

	py::class_<RET_ERRC> (m, "RET_ERRC")
	
	.def("__repr__", &__repr__)
	
	.def("__bool__", [] (RET_ERRC a) -> bool {
		return a.flags != ERR_CODE::SUCCESS;
	})
	
	.def("__add__",  [] (RET_ERRC a, RET_ERRC b) -> RET_ERRC {
		return RET_ERRC{a.flags | b.flags};
	})
	
	.def("__call__", [] (const RET_ERRC & e) -> void {
		if (e.flags) {
			throw std::runtime_error (__repr__(e));
		}
	}, "Throws exception if error")
	
	/* end RET_ERRC class */;
}
