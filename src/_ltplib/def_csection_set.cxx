#include "def_csection_set.hxx"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>


/******************************************************************************/
csection_set_holder::~csection_set_holder (void) {
	logger::debug ("delete csection_set");
}

/******************************************************************************/
csection_set_holder:: csection_set_holder ( \
std::vector<py::dict> _entries,
f32                 _max_energy,
py::str               _ptdescr,
py::str               _bgdescr,
py::kwargs            kws
)
: cfg{std::make_unique<csection_set_cfg>
	(_entries, _max_energy, _ptdescr, _bgdescr, (py::dict)(kws))}
{
	m.data_holder
		.req(&(this->progs), cfg->progs.size())
		.req(&(this->cffts), cfg->cffts.size())
		.req(&(this->tabs),  cfg->tabs.size())
		.alloc();
		
	memcpy
	(this->progs, cfg->progs.data(), (cfg->progs.size())*sizeof(mprog_t));
	memcpy
	(this->cffts, cfg->cffts.data(), (cfg->cffts.size())*sizeof(f32));
	memcpy
	(this->tabs,  cfg->tabs.data(),  (cfg->tabs.size()) *sizeof(f32));
	this->tsize      = cfg->tsize;
	this->ncsect     = cfg->ncsect;
	this->max_energy = cfg->max_energy;
	
	logger::debug("__init__ csection_set ({} KiB)",
	m.data_holder.get_size()/1024.0);
}

/******************************************************************************/
const char *CSECTION_SET_DESCR {
R"pbdoc(Cross-section database for Monte-Carlo simulations.

Input cross-sections will be recalculated into cummulative rates and stored
into the lookup-table, with log-scaled energy grid:
$$\varepsilon = \varepsilon_{\rm th} + 2^{j/2-4}-0.0625,~j\in\mathbb{N},$$
where $\varepsilon_{\rm th}$ is a reaction threshold.
)pbdoc"};

const char *CSECTION_SET_INIT {
R"pbdoc(Creates coress-cestion database.

Parameters
----------

cfg : Configuration sequence to describe all the processes.
The following example contains all possible variants for configuration entries:
[
  # The first entry should describe an active component ("e" in this example).
  {"TYPE":"SELECTPT", "KEY":"e",
   # coefficient to transform speed² -> energy (eV)
   "ENCFFT":2.842815e-16
  },
  
  # The second one describes the background to interact with.
  {"TYPE":"SELECTBG", "KEY":"CH4",
   # this describes m/M ratio required for elastic collisions)
   "MASSRATE":3.420074e-05
  },
  
  # The next entries show the different cross-sections' descriptions
  # The following types are supported:
  # ELASTIC|EXCITATION|VIBRATIONAL|ROTATIONAL|DISSOCIATION|IONIZATION|ATTACHMENT
  {"TYPE":"ELASTIC",
  
   # Cross-section could be described as a:
   # [*] python function
   "CSEC": lambda en: 1e-15*(en-1)/((en-1)**2+1),
   
   # [*] list of energy-&-value pairs or numpy array
   "CSEC": [(2.0, 6.0E-20), (3.0, 6.0E-20), (10.0, 8.0E-20), ...],
   
   # [*] text file containing energy-&-value pairs, separated by space 
   "CSEC": "database/O2_ELASTIC.txt",
   
   # [*] Also possible to pass additional parameters using this form:
   "CSEC": (data, {**params}), # see also **kwargs
   # Log-log interpolation will be used to map data-samples into the lookup-table.
   # High-energy values will be extrapolated, assuming that that
   # cross-section can be described by decaying exp-function.
   # Extrapolation coefficients are calculated automatically using
   # samples for with log10(en_last) - log10(en) <= exterp.
   # The latter value can be passed as a paremeter.
   # By default exterp = 0.25, use exterp = 0 to turn off the extrapolation.
   # To rescale input data or function use 'scale' parameter.
   # To start reading from specific line use 'lineno'  parameter.
   # Note, the lines start with '#', '!', '%', '//' will be ignored.
   
   # Also note, it is possible to read data directly from LXCat-file using
   # "search":"pattern" in **params. The data will be parsed from
   # "-----"//"-----" block after the first pattern's occurence.
   
   # By default isotropic angular scattering is assumed for
   # all conservative collisions. However, first-order approximation
   # can be used to simulate anisotropic scattering.
   # In this case field CSEC will be interpteted as ICS (Integral Cross Section)
   # There are two variantsto do this:
   
   # [*] by passing momentum-transfer cross-section
   "MTCS": data,
   # Corresponding appriximation for DCS (Differential Cross Section) then
   # will be calculated from MTCS/CSEC ratio,
   # see ref. [M Flynn et al 2024 J. Phys. D: Appl. Phys. 57 255204.
   # Benchmark calculations for anisotropic scattering in  kinetic models for
   # low temperature plasma. doi:10.1088/1361-6463/ad3477].
   
   # [*] or py passig energy-depended fitting parameter directly as a function:
   "DCSFN" : lambda en, es=10: 4*en/es(1+4*en/es) # screen Coulomb
   # Note, here "en" means energy minus reaction's threshold.
   # The function sould lie in a range of ±1, where
   # value +1 means pure forward scattering, and -1 means backward scattering.
  },
  
  # Energy threshold is necessary for inelastic collisions (except ATTACHMENT).
  {"TYPE":"EXCITATION", "THRESHOLD":8.8, ...},
  
  # Ionization collisions are always anisotropic.
  # Scattering angles for incidend and secondaty particle(s) are calculated
  # using energy- and impulse-conservation laws.
  {"TYPE":"IONIZATION", "THRESHOLD":12.4,
   "CSEC": data,
   # Energy sharing between incedent and secondary particle(s) is described
   # by OPB-appriximation, see ref. [Opal, Peterson, Beaty
   # J. Chem. Phys. 55, 4100 (1971), doi.org/10.1063/1.1676707]
   "OPBPARAM": 7.3,
   # in absence of this parmeter "THRESHOLD" will be used instead.
   
   # list of new spawned (active) particles should be presented also
   "SPAWN":"e CH4+" # [TBD], do not use it now!
  },
]

max_energy : defines energy limit and size of the looku  p-table.

ptdescr : the string to describe all the active components separated by spaces.
  Any other component from the configuration sequence will be omitted.
  
bgdescr : the string to describe all the background components separated by spaces. 
  In case of empty string it will be ignored and background components will be
  taken into account according to configuration sequence.
  In case of non-empty string, only listed components will be used.
  Also, additional flags could be assigned to background after '/' symbol,
  for example: // [TBD] do not use them yet!
   "CH4/t" takes into account background's thermal speed,
   "CH4/vt" also takes into account background's flow velocity

**kwargs can be used to pass some global default parameters:
  sacle, exterp : same as for the individual entry,
  debug : prints additional info during the building of the lookup-table 
)pbdoc"};

/******************************************************************************/
void def_csections(py::module &m) {
	
	py::class_<csection_set_holder> cs_cls (m, "csection_set",
	CSECTION_SET_DESCR);
	
	py::class_<db_entry_t> entry_cls (cs_cls, "db_entry",
	"cross-section database entry");
	
	/* start cross-section database class */ cs_cls
	
	.def(py::init<std::vector<py::dict>, f32, py::str, py::str, py::kwargs>
	(), "cfg"_a, "max_energy"_a, "ptdescr"_a="e", "bgdescr"_a=""s
	, CSECTION_SET_INIT)
	
	.def_readonly("max_energy", &csection_set_holder::max_energy,
	"energy limit")
	
	.def_property_readonly("chinfo", [] (const csection_set_holder& self) {
		return self.cfg->chinfo;
	}, "reaction channels' description"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("ptlist", [] (const csection_set_holder& self) {
		return self.cfg->ptinfo;
	}, "list of active components"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("bglist", [] (const csection_set_holder& self) {
		return self.cfg->bginfo;
	}, "list of background components"
	, py::keep_alive<0, 1>())

	.def_property_readonly("points", [] (const csection_set_holder& self) {
		return py::array_t<f32> (py::memoryview::from_buffer (
			/* ptr      */ self.cfg->points.data(),
			/* shape    */ {self.cfg->points.size()},
			/* strides  */ {sizeof(f32)},
			/* readonly */ true
		));
	}, "energy-grid used in the lookup-table"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("cffts", [] (const csection_set_holder& self) {
		return py::array_t<f32> (py::memoryview::from_buffer (
			/* ptr      */ self.cfg->cffts.data(),
			/* shape    */ {self.cfg->cffts.size()},
			/* strides  */ {sizeof(f32)},
			/* readonly */ true
		));
	}, "internal cache for various coefficients being used"
	, py::keep_alive<0, 1>())

	.def_property_readonly("tabs", [] (const csection_set_holder& self) {
		std::vector<py::ssize_t> shape(2), strides(2);
		shape  [1] = self.cfg->tsize;
		strides[1] = sizeof(f32);
		shape  [0] = self.cfg->ncsect+self.cfg->nxtra;
		strides[0] = shape[1]*strides[1];
		
		return py::array_t<f32> (py::memoryview::from_buffer (
			/* ptr      */ &self.cfg->tabs[self.cfg->ncsect],
			/* shape    */ std::move(shape),
			/* strides  */ std::move(strides),
			/* readonly */ true
		));
	}, "internal cache for the lookup-table"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("progs", [] (const csection_set_holder& self) {
		std::vector<std::string> progs_repr;
		for (const auto& cmd : self.cfg->progs) {
			if (opcode::END == cmd.opc) {
				progs_repr.push_back \
				(fmt::format("{}", MPROG_DESCR[cmd.opc]));
			} else {
				progs_repr.push_back \
				(fmt::format("{}.{}", MPROG_DESCR[cmd.opc], cmd.arg));
			}
		}
		return progs_repr;
	}, "configuration sequence"
	, py::keep_alive<0, 1>())
	
	.def("__len__", [] (const csection_set_holder& self) {
		return self.cfg->ncsect;
	}, "number of reaction channels")

	.def_property_readonly("db_entries", [] (const csection_set_holder& self) {
		return self.cfg->db_entries;
	}, "database entries for diagnostic/visualization"
	, py::keep_alive<0, 1>())
	
	.def("__getitem__", [] (const csection_set_holder& self, u16 k) {
		if (k < self.cfg->db_entries.size()) {
			return self.cfg->db_entries[k];
		} else throw \
		py::index_error(fmt::format("{} >= {}", k, self.cfg->db_entries.size()));
	}, "channel_id"_a, "returns specific entry, same as db_entries[channel_id]"
	, py::keep_alive<0, 1>())
	
	.def("__iter__", [] (const csection_set_holder& self) {
		return py::make_iterator(
			self.cfg->db_entries.begin(), self.cfg->db_entries.end()
		);
	}, "iterate over db_entries"
	, py::keep_alive<0, 1>())
	
	/* end cross-section class */;
	
	
	/* start database entry class */ entry_cls
	.def_readonly("descr", &db_entry_t::descr
	, "channel's description")

	.def ("__str__",
	[] (const db_entry_t& self) -> std::string {
		return self.descr;
	})
	
	.def_readonly("enth", &db_entry_t::enth,
	"energy threshold")
	
	.def_readonly("rate_max", &db_entry_t::rmax,
	"null-collision cumulative rate")
	
	.def_property_readonly("rate_fn", [] (const db_entry_t& self) -> csfunc_t {
		return self.fns.at("C_RATE");
	}, "function for energy-depended cumulative rate"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("csec_fn", [] (const db_entry_t& self) -> csfunc_t {
		return self.fns.at("CS0");
	}, "function for cross-section"
	, py::keep_alive<0, 1>())
	
	.def_property_readonly("mtcs_fn",
	[] (const db_entry_t& self) -> std::optional<csfunc_t> {
		if (self.fns.contains("CS1")) {
			return self.fns.at("CS1");
		} else {
			return std::nullopt;
		}
	}, "function for momentum-transfer cross-section (or None)"
	, py::keep_alive<0, 1>())
	
	.def ("__getattr__",
	[] (const db_entry_t& self, py::str key) -> std::optional<py::object> {
		if (self.extra.contains(key)) {
			return self.extra[key];
		} else {
			return std::nullopt;
		}
	})
	
	/* end database entry class */;
}

