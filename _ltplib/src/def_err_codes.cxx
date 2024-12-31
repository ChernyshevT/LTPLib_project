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
// here was numpy...

#include "api_backend.hxx"

std::string __repr__(const RET_ERRC & e) {
	std::string msg;
	switch (e.code) {
		break; case ERR_CODE::SUCCESS:
			return "";
		break; case ERR_CODE::PPOST_ERR:
			msg = "PPOST_ERR";
		break; case ERR_CODE::PPUSH_ERR:
			msg = "PPUSH_ERR";
		break; case ERR_CODE::PORDER_ERR:
			msg = "PORDER_ERR";
		break; case ERR_CODE::PMCSIM_ERR:
			msg = "PMCSIM_ERR";
		break; case ERR_CODE::PCHECK_ERR:
			msg = "PCHECK_ERR";
		break; default:
			msg = "INVALID_SEQ";
	}
	if (e.flags & ERR_FLAG::OVERFLOW) {
		msg = fmt::format("{}|OVERFLOW", msg);
	}
	if (e.flags & ERR_FLAG::OUTOFRANGE) {
		msg = fmt::format("{}|OUTOFRANGE", msg);
	}
	if (e.flags & ERR_FLAG::NANVALUE) {
		msg = fmt::format("{}|NANVALUE", msg);
	}
	if (e.flags & ERR_FLAG::ENERGYMAX) {
		msg = fmt::format("{}|ENERGYMAX", msg);
	}
	if (e.flags & ERR_FLAG::PROBMAX) {
		msg = fmt::format("{}|PROBMAX", msg);
	}
	if (e.flags & ERR_FLAG::RUNAWAYPT) {
		msg = fmt::format("{}|RUNAWAYPT", msg);
	}
	if (e.flags & ERR_FLAG::FASTPT) {
		msg = fmt::format("{}|FASTPT", msg);
	}
	return msg;
}

void def_err_codes(py::module &m) {

	py::class_<RET_ERRC> cls (m, "RET_ERRC");
	
	cls.def_readonly("dtime", &RET_ERRC::dtime);
	
	cls.def("__repr__", &__repr__);
	
	cls.def("__bool__", [] (RET_ERRC a) -> bool {
		return a.code != ERR_CODE::SUCCESS;
	});
	
	cls.def("__add__",  [] (RET_ERRC a, RET_ERRC b) -> RET_ERRC {
		if (a.code == b.code and a.code != ERR_CODE::INVALID_SEQ) {
			return {a.code, u8(a.flags|b.flags)};
		} else {
			return {ERR_CODE::INVALID_SEQ, u8(a.flags|b.flags)};
		}
	});
	
	cls.def("__call__", [] (const RET_ERRC & e) -> RET_ERRC {
		std::string msg{__repr__(e)};
		
		switch (e.code) {
			
			case ERR_CODE::SUCCESS:
				break;
			
			case ERR_CODE::PCHECK_ERR:
				if (e.flags & ERR_FLAG::OUTOFRANGE) {
					throw std::runtime_error (msg);
				}
				if (e.flags & ERR_FLAG::RUNAWAYPT) {
					logger::error("{}!", msg);
					break;
				}
				if (e.flags & ERR_FLAG::FASTPT) {
					logger::warning("{}!", msg);
					break;
				}
			
			default:
				throw std::runtime_error (msg);
		}
		return e;
	}, "Throws exception or warning");
}
