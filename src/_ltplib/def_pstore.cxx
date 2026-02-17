#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
namespace py = pybind11;
using namespace pybind11::literals;
#include <unordered_map>
#include <climits>

#include "io_memory.hxx"
#include "io_strings.hxx"
#include "api_pstore.hxx"
#include "api_grid.hxx"
#include "def_pstore.hxx"

/******************************************************************************/
template<u8 nd> inject_fn_t construct_inject_fn
(pstore_holder& self, const grid_t<nd>& grid) {

	return [&] (u8 tag, const parts_input_t& input) -> size_t {
		auto pts{input.unchecked<2>()};
		if (pts.shape(1) != nd+3) {
			throw std::invalid_argument(std::format("pts.shape[1] != {}+3", nd));
		}
		
		size_t n_overflow{0};
		
		for (int64_t j{0}, npp{pts.shape(0)}; j<npp; ++j) {
			f32 pt[nd+3]; bool valid{true};
			for (auto i{0}; i<nd+3; ++i) {
				pt[i] = pts(j, i);
				valid = valid or isfinite(pt[i]);
			}
			
			if (u32 k{grid.find_node(pt)}; valid and k != 0) {
				auto pool{self[k-1]};

				if (u32 j{pool.index[0]}; j < pool.index[1]) {
					for (u32 i{0}; i<nd+3; ++i) {
						pool.parts[j][i+1].vec    = pt[i];
					} pool.parts[j][0  ].tag[0] = tag;
					++pool.index[0];
				} else {
					n_overflow++;
				}
			} //else skip
		}
		
		return n_overflow;
	};
}

template<u8 nd> extract_fn_t construct_extract_fn
(const pstore_holder& self, const grid_t<nd>& grid) {
	
	return [&] (void) -> std::tuple<parts_output_t, std::vector<size_t>> {
		
		// run #1 count particles [TODO: into separate fn!]
		std::vector<size_t> counter(self.opts.ntypes+2, 0);
		
		for (u32 k{0}; k<grid.size; ++k) {
			auto pool{self[k]};
			
			for (u32 j{0}; j<pool.index[0]; ++j) {
				u8 tag{pool.parts[j][0].tag[0]};
				counter[2+tag] += 1;
			}
		}
		for (u32 i{0}; i<self.opts.ntypes; ++i) {
			counter[2+i] += counter[1+i];
		}
		
		
		std::vector<py::ssize_t> shape(2);
		shape[0] = counter[self.opts.ntypes+1];
		shape[1] = nd+3;
		
		//~ py::print("counter", counter);
		//~ py::print("shape", shape);
		
		// run #2 copy  particles [TODO: into separate fn!]
		parts_output_t output(std::move(shape));
		auto pts{output.mutable_unchecked<2>()};
		
		for (u32 k{0}; k<grid.size; ++k) {
			auto pool{self[k]};
			
			for (u32 j{0}; j<pool.index[0]; ++j) {
				u8  tag{pool.parts[j][0].tag[0]};
				
				for (u8 i{0}; i<nd+3; ++i) {
					pts(counter[tag+1], i) = pool.parts[j][i+1].vec;
				}
				counter[tag+1] += 1;
			}
		}
		counter.resize(self.opts.ntypes+1);
		
		//~ py::print("counter", counter);
		//~ py::print("output", output);
		
		//~ exit(1);
		
		return {std::move(output), std::move(counter)};
	};
}

template<u8 nd> reset_fn_t construct_reset_fn
(pstore_holder& self, const grid_t<nd>& grid) {
	
	return [&] (void) {
		for (auto k{0u}; k<grid.size; ++k) {
			auto pool{self[k]};
			for (auto i{0u}; i<self.cfg->idxsh; ++i) {
				pool.index[i] = pool.npmax*(i>0);
			}
		}
	};
}

/******************************************************************************/

#define NPPMAX_LIMIT (1<<24)

pstore_cfg::pstore_cfg
( const grid_holder       *gridp
, std::vector<py::dict>    ptinfo_i
, u32                      npmax_i
, u8                       nargs_i
, py::dict
) {

	for (auto entry : ptinfo_i) {
		auto key = py::cast<std::string>(entry["KEY"]);
		auto cff = py::cast<f32>(entry["CHARGE/MASS"]);
		cffts .push_back(cff);
		ptinfo.push_back(key);
	}
	
	u32 nd{0}, md{0}, sz{1};

	
	nd = gridp->cfg->shape.size();
	sz = gridp->cfg->nodes.size();
	md = 1; for (auto i{0u}; i<nd; ++i) md*=3;

	nargs = nargs_i != 0? nargs_i : 1+nd+3;
	npmax = npmax_i;
	ntype = ptinfo.size();
	
	if (npmax >= NPPMAX_LIMIT) throw \
	std::invalid_argument(std::format("capacity: {} >= {}", npmax, NPPMAX_LIMIT));
	
	
	if (nargs <= nd+3) throw \
	std::invalid_argument(std::format("vsize: {} <= {}", nargs, nd+3));
	
	idxsh  = 1+md;
	npools = sz;
	
	pparts_sz = sz*npmax*nargs;
	pindex_sz = sz*idxsh;
	pflags_sz = sz*(1+2*npmax);
}

struct pstore_ctor {
	pstore_holder  &holder;

	void init () {
		pstore_t &pstore{holder};
		
		holder.m.pparts_h
		.req(&pstore.cffts,  holder.cfg->ntype)
		.req(&pstore.ppdata, holder.cfg->pparts_sz)
		.alloc();
		
		holder.m.pindex_h
		.req(&pstore.pindex, holder.cfg->pindex_sz)
		.req(&pstore.queue,  holder.cfg->npools)
		.alloc();

		holder.m.pflags_h
		.req(&pstore.pflags, holder.cfg->pflags_sz)
		.alloc();
		
		pstore.nargs = holder.cfg->nargs;
		pstore.npmax = holder.cfg->npmax;
		pstore.opts.ntypes  = holder.cfg->ntype;
		pstore.opts.idshift = holder.cfg->idxsh;
		pstore.opts.ongpu   = 0;
		pstore.opts.mode    = 0;
	}

	template <u8 nd, u8 md=1 + (nd==1? 3 : nd==2? 9 : 27)>
	void operator () (const grid_t<nd> &grid) {
		logger::debug
		("construct pstore ({}, &grid = {})", (void*)(&holder), (void*)(&grid));
		pstore_t &pstore{holder};
		
		init();
		
		for (auto i{0u}; i<holder.cfg->ntype; ++i) {
			pstore.cffts[i] = holder.cfg->cffts[i];
		}
		for (auto i{0u}; i<holder.cfg->npools; ++i) {
			pstore.queue[i] = i;
		}
		

		holder.inject_fn  = construct_inject_fn  (holder, grid);
		holder.extract_fn = construct_extract_fn (holder, grid);
		holder.reset_fn   = construct_reset_fn   (holder, grid);
		
		holder.count_fn = [&] (void) -> size_t {
			size_t n{0};
			for (u32 k{0}; k < grid.size; ++k) {
				n += pstore[k].index[0];
			}
			return n;
		};
		
		holder.reset_fn();
	}
};

pstore_holder:: pstore_holder (const grid_holder& grid, std::vector<py::dict> ptinfo_i, u32 npmax_i, u8 nargs_i, py::kwargs kwargs)
: gridp{&grid}
, cfg{std::make_unique<pstore_cfg>(&grid, ptinfo_i, npmax_i, nargs_i, py::dict{**kwargs})}
{
	std::visit(pstore_ctor{*this}, grid);
	{
		auto sz{f64(
			m.pparts_h.get_size() +
			m.pindex_h.get_size() +
			m.pflags_h.get_size()
		)};
		u8 i{0}; 
		const char* lab[] = {"","Ki","Mi","Gi","Ti"};
		while (sz >= 512.0) {
			sz /= 1024.0; ++i;
		}
		logger::debug("create pstore ({}, {:.3f} {}B)", (void*)(this), sz, lab[i]);
	}
}

pstore_holder::~pstore_holder (void) {
	logger::debug("delete pstore ({})", (void*)(this));
}
/******************************************************************************/
const char *PSTORE_INIT {
R"pbdoc(Creates particles storage.

Parameters
----------
grid : existing grid,
cfg : list containing components' description, i.e.:
	[
	 {"KEY":"e", "CHARGE/MASS": -5.333333e+17},
	 {"KEY":"i", "CHARGE/MASS": +2.201710e+12},
	],
capacity -- maximum number of samples per node 
vsize -- (optional) sample vector size:
	1+nd+3 (by default)
	or
	1+(nd+3)*2 in case of semi-implicit mover
)pbdoc"
};

const char *INJECT_FN {
R"pbdoc(Injects particles into the storage from the external arrays

Parameters
----------
input : dictionary with particles to inject
  each key  should correspond to self.ptlist
  each item should be array-like with shape == [:, nd+3]
)pbdoc"
};

const char *EXTRACT_FN {
R"pbdoc(Saves particles into external array.

Returns
-------
pair (output, index):
  output: numpy.ndarray[numpy.f32]
    pVDF samples, shape == [:, nd+3]
  index: List[int]]
    range where specific component is located,
    for example, if ptlist=["e", "Xe+", "Ar+"]
    then samples for "Xe+" are in output[index[1]:index[2]]
)pbdoc"
};

/******************************************************************************/
void def_pstores (py::module &m) {

	py::class_<pstore_holder> cls (m, "pstore",
	"Particle storage"); cls
	
	.def(py::init\
	<const grid_holder&, std::vector<py::dict>, u32, u8, py::kwargs> ()
	, "grid"_a, "cfg"_a, "capacity"_a, "vsize"_a=0, PSTORE_INIT
	, py::keep_alive<0, 1>() /* keep grid */
	)

	.def_property_readonly("ptlist", [] (const pstore_holder& self) {
		return self.cfg->ptinfo;
	}, "returns list of components")
	
	.def("inject", [] (pstore_holder &self
	, const std::string& key,  const parts_input_t& data) {
	//~ , const std::unordered_map<std::string, parts_input_t> &input) {
		size_t n_overflow{0};
		//~ for (const auto& [key, data] : input) {

		auto pos{std::find(self.cfg->ptinfo.begin(), self.cfg->ptinfo.end(), key)};
		if (pos == self.cfg->ptinfo.end()) {
			throw std::invalid_argument(std::format("invalid key \"{}\"", key));
		}
			
		n_overflow = self.inject_fn(u8(pos-self.cfg->ptinfo.begin()), data);
		if (n_overflow) {
			throw std::overflow_error(std::format(
			"pstore.inject: can not inject {} samples: overflow!", n_overflow));
		}
	}, "key"_a, "input"_a, INJECT_FN)
	
	.def("extract", [] (const pstore_holder &self) {
		return self.extract_fn();
	}, EXTRACT_FN)
	
	.def("__len__", [] (const pstore_holder& self) {
		return self.count_fn();
	}, "returns the number of samples stored in")
	
	.def("reset", [] (pstore_holder& self) {
		return self.reset_fn();
	}, "makes pstore empty")

	/* end class */;
}
