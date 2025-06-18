#include "def_grid.hxx"

//https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/
//https://medium.com/@nerudaj/std-visit-is-awesome-heres-why-f183f6437932

struct grid_ctor {
	grid_holder &holder;
	
	template <u8 nd, u8 md=((nd>1?(nd>2?27:9):3)-1)>
	void operator () (grid_t<nd> &grid) {
		
		logger::debug("construct grid{} ({})",
		nd, (void*)(&grid)
		);
		
		// prepare allocator
		for (u8 i{0}; i<nd; ++i) {
			holder.m.base_h.req(&(grid.axes [i]), holder.cfg->shape[i]+1);
			holder.m.base_h.req(&(grid.edges[i]), holder.cfg->shape[i]+1);
		}
		holder.m.base_h.req(&(grid.nodes), holder.cfg->nodes.size());
		if (holder.cfg->mask.size() > 0) {
			holder.m.base_h.req(&(grid.mask), holder.cfg->mask.size());
		} else {
			grid.mask = nullptr;
		}
		holder.m.base_h.req(&(grid.lctr), holder.cfg->lctr.size());
		// allocate all
		holder.m.base_h.alloc();
		
		// copy stuff
		for (u8 i{0}; i<nd; ++i) {
			for (size_t j{0}; j<=holder.cfg->shape[i]; ++j) {
				grid.axes [i][j] = holder.cfg->axes [i][j];
				grid.edges[i][j] = holder.cfg->edges[i][j];
			}
			grid.step [i] = holder.cfg->step[i];
			grid.shape[i] = holder.cfg->shape[i];
		}
		for (size_t k{0}; k<holder.cfg->nodes.size(); k++) {
			for (u8 i{0}; i<nd; ++i) {
				grid.nodes[k].map[i] = holder.cfg->nodes[k].map[i];
			}
			for (u8 i{0}; i<md; ++i) {
				grid.nodes[k].lnk[i] = holder.cfg->nodes[k].lnk[i];
			}
			grid.nodes[k].mshift  = holder.cfg->nodes[k].mshift;
		}
		for (size_t j{0}; j<holder.cfg->mask.size(); ++j) {
			grid.mask[j] = holder.cfg->mask[j];
		}
		for (size_t j{0}; j<holder.cfg->lctr.size(); ++j) {
			grid.lctr[j] = holder.cfg->lctr[j];
		}
		grid.flags.loopax = holder.cfg->flags["loopax"];
		grid.flags.cylcrd = holder.cfg->flags["cylcrd"];
		grid.size         = holder.cfg->nodes.size();
		
		//~ holder.find_node_fn = [&grid] (std::vector<f32>&& pos) {
			//~ return grid.find_node(&pos[0]);
		//~ };
	}
};

//~ grid_holder:: grid_holder (u8 nd, py::dict arg, py::kwargs kwargs)
grid_holder:: grid_holder
(u8 nd, step_a _step, axes_a _axes, nodes_a _nodes, mask_a _mask, py::kwargs kwargs)
: grid_v {
		nd == 1? grid_v{grid_t<1>{}} :
		nd == 2? grid_v{grid_t<2>{}} :
		nd == 3? grid_v{grid_t<3>{}} :
		throw std::invalid_argument(fmt::format("nd == {}", nd))
	}
, cfg {std::make_unique<grid_cfg>
	(nd, py::dict{"step"_a=_step, "axes"_a=_axes, "nodes"_a=_nodes, "mask"_a=_mask , **kwargs})}
{
	//~ cfg = std::make_unique<grid_cfg> 
	
	std::visit(grid_ctor{*this}, *this);
	
	{
		u8 i{0}; f64 sz{f64(m.base_h.get_size())};
		const char* lab[] = {"","Ki","Mi","Gi","Ti"};
		while (sz >= 512.0) {
			sz /= 1024.0; ++i;
		}
		logger::debug("create grid ({}, {:.3f} {}B)", (void*)(this), sz, lab[i]);
	}
}

grid_holder::~grid_holder (void) {
	logger::debug("delete grid ({})", (void*)(this));
};

/******************************************************************************/
void def_grids (py::module &m) {

	py::class_<grid_holder> cls (m, "grid", 
	R"pbdoc(Class to use as a base for any spatial-problems.
	It describes problem's geometry, and nodes for parallel computation.
	)pbdoc"); cls
	
	.def(py::init<u8, step_a, axes_a, nodes_a, mask_a, py::kwargs>
	(), "nd"_a, "step"_a, "axes"_a, "nodes"_a, "mask"_a=std::nullopt,
	R"pbdoc(Creates grid class --- problem's geometry and base of domain decomposition.
	
	Parameters
	----------
	
	nd : number of spatial dimensions
	
	cfg : dictionary desctibing problem's geometry.
	Here is an example for the 2d problem:
	[
		# First, let's define spatial-step for each dimension
		"step": [0.25, 0.25], # x, y
		
		# The next two sections describe domain decomposition.
		# Here, the numbers inbetween define edges of sub-domains:
		"axes": [
		 # x-axis, 2 sections
		 [0, 16, 32],
		 # y-axis, 3 sections
		 [0, 20, 40, 60], 
		],
		
		# Now the position of each sub-domain is described relative to the "axes":
		"nodes": [
			(0, 0), # (0 <= x < 16), (0  <= y < 20)
			(0, 1), # (0 <= x < 16), (20 <= y < 40)
			...
		]
		# The links between the nodes will be builded automatically.
		
		# It is possible to mark some cells as a particle adsorbers using
		# following optional parameter:
		"mask" : [...] # uint8 numpy array, with the same shape as grid axes.
		
		# For 2d axisymmetric problems it one axes should be marked:
		"cylcrd": 'x'
		
		# Axes can be marked as looped to their self:
		"loopax": "x",
	]
	)pbdoc")
	
	.def("find_node", [] (const grid_holder& self, std::vector<f32> pos) -> u32 {
		
		return std::visit([&] <u8 nd> (const grid_t<nd> &grid) {
			return grid.find_node(&pos[0]);
		}, self);
		
	})
	
	.def_property_readonly("ndim", [] (const grid_holder& self) {
		return self.cfg->shape.size();
	})
	.def_property_readonly("shape", [] (const grid_holder& self) {
		return self.cfg->shape;
	})
	.def_property_readonly("units", [] (const grid_holder& self) {
		return self.cfg->units;
	})
	.def_property_readonly("step",  [] (const grid_holder& self) {
		return self.cfg->step;
	})
	.def_property_readonly("axes",  [] (const grid_holder& self) {
		return self.cfg->axes;
	})
	.def_property_readonly("edges", [] (const grid_holder& self) {
		return self.cfg->edges;
	})
	.def_property_readonly("lctr",  [] (const grid_holder& self) {
		return self.cfg->lctr;
	})
	.def_property_readonly("flags", [] (const grid_holder& self) {
		py::dict dict;
    for (auto [k,v] : self.cfg->flags) {
			dict[py::str(k)] = v;
    }
		return py::module::import("types").attr("SimpleNamespace")(**dict);
	});

}
