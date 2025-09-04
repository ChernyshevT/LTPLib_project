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

struct db_group_t;
struct db_entry_t;
struct csection_set_cfg;

	enum db_group_flags : u8 {
	DESCR_DEF = 0,
	MRATE_DEF,
	VTERM_DEF,
	VFLUX_DEF,
	GROUP_FLAGSNUM,
};
typedef std::bitset<GROUP_FLAGSNUM> group_flags_t;

struct db_group_t {
	std::string    bgkey;
	std::string    descr;
	f32            massrate{0.0f};
	u16            pt_index;
	u16            bg_index;
	u16            ch_index[2];
	group_flags_t  flags;

	db_group_t (csection_set_cfg *cfg, py::dict entry);
};

enum db_entry_flags : u8 {
	ENTH_DEF = 0,
	CSEC_DEF,
	MTCS_DEF,
	DCSFN_DEF,
	OPBPARAM_DEF,
	PRODUCTS_DEF,
	ENTRY_FLAGSNUM,
};
typedef std::bitset<ENTRY_FLAGSNUM> entry_flags_t;

struct db_entry_t {
	opcode         opc;
	std::string    descr;
	f32            enth;
	f32            rmax;
	py::dict       extra;
	std::unordered_map<std::string, csfunc_t> fns; 
	entry_flags_t  flags;
	
	db_entry_t (const csection_set_cfg *cfg, py::dict entry, py::dict opts);
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
	std::vector<f32>              consts;
	std::vector<f32>              _tabs;
	std::vector<u16>              _index;
	std::vector<db_group_t>       db_groups;
	std::vector<db_entry_t>       db_entries;
	
	std::vector<std::string>
		pt_list,
		bg_list;
	std::map<std::string, group_flags_t>
		bg_flags;
	
	csection_set_cfg
	(std::vector<py::dict>, f32, py::str, py::str, py::dict);
};

void add_particle(csection_set_cfg *cfg, py::dict obj);
void add_db_group(csection_set_cfg *cfg, py::dict obj);
void add_db_entry(csection_set_cfg *cfg, py::dict obj, py::dict opts);
u16  add_constant(csection_set_cfg *cfg, f32 arg);

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
