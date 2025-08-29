#ifndef _DEF_CSECTIONS_HEADER
#define _DEF_CSECTIONS_HEADER

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
namespace py = pybind11; using namespace pybind11::literals;
#include "typedefs.hxx"
#include "api_csection_set.hxx"
#include "io_strings.hxx"
#include "io_memory.hxx"

#include <memory>
#include <functional>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <fstream>
#include <regex>
#include <bitset>
#include <unordered_map>

typedef std::function<f32(f32)> csfunc_t;

struct pt_entry_t;
struct db_group_t;
struct db_entry_t;
struct csection_set_cfg;

struct pt_entry_t {
	std::string name;
	f32 encfft;
	u16 group_index;
	
	pt_entry_t (csection_set_cfg *cfg, py::dict entry);
};

struct db_group_t {
	std::string name;
	f32 massrate{0.0f};
	u16 channel_index;
	//~ u16 pt_uid        = 0;
	//~ u16 ma_uid        = 0;
	
	//~ u16 ch_xtra       = 0;
	//~ u16 bg_idx        = 0;
	//~ u16 bg_therm_mark = 0;
	//~ u16 bg_flux_mark  = 0;
	
	db_group_t (csection_set_cfg *cfg, py::dict entry);
};

struct db_entry_t {
	opcode         opc;
	std::bitset<5> FLAGS{0};
	//std::vector<std::string> products;
	std::string    descr;
	f32            enth;
	f32            rmax;
	py::dict       extra;
	std::unordered_map<std::string, csfunc_t> fns; 
	
	db_entry_t (csection_set_cfg *cfg, py::dict entry, py::dict opts);
};

/******************************************************************************/
struct csection_set_cfg {
	typedef mprog_t::opc_t opcode;
	
	std::vector<std::string>      ptinfo, bginfo, chinfo;
	f32                         max_energy;
	std::vector<mprog_t>          progs;
	std::vector<f32>            cffts, points, cstabs, rates, tabs;
	std::map<u16, py::tuple> dset;
	u8                       tsize,  ntype; 
	u16                      ncsect, nxtra;
	
	//~ std::unordered_map<std::string>
	std::vector<f32>              _consts;
	std::vector<f32>              _tabs;
	std::vector<u16>              _index;
	std::vector<pt_entry_t>       pt_entries;
	std::vector<db_group_t>       db_groups;
	std::vector<db_entry_t>       db_entries;
	
	csection_set_cfg
	(std::vector<py::dict>, f32, py::str, py::str, py::dict);

};

/******************************************************************************/
class csection_set_holder : public csection_set_t {

	struct {
		mem_holder data_holder;
	} m;

public:

	std::unique_ptr<csection_set_cfg> cfg;
	
	~csection_set_holder (void);
	
	 csection_set_holder
	(std::vector<py::dict>, f32, py::str, py::str, py::kwargs);
};

#endif
