#include "def_grid.hxx"

//https://arne-mertz.de/2018/05/overload-build-a-variant-visitor-on-the-fly/
//https://medium.com/@nerudaj/std-visit-is-awesome-heres-why-f183f6437932

static
u8 parse_grid_flags(const char* mode_str) {
	constexpr struct {
		const char* key;
		u8          val;
	} table[] = {
		{"CYLINDER", AXIS_FLAG::CYLINDER},
		{"LOOPX",    AXIS_FLAG::LOOPX},
		{"LOOPY",    AXIS_FLAG::LOOPY},
		{"LOOPZ",    AXIS_FLAG::LOOPZ},
	};
	constexpr u8 n{sizeof(table)/sizeof(*table)};

	bool matched, used[n]{false};
	u8 flags = 0;
	
	while (*mode_str) {
		if (std::isspace(*mode_str) or '|' == *mode_str) {
			mode_str += 1;
			continue;
		}
		matched = false;
		for (u8 i{0u}; i<n; ++i) {
			size_t len = std::strlen(table[i].key);
			if (std::strncmp(mode_str, table[i].key, len) == 0) {
				if (used[i]) {
					throw bad_arg("flag duplication: \"{}\"", table[i].key);
				}
				used[i] = true;
				matched = true;
				flags = flags | table[i].val;
				mode_str += len;
				break;
			}
		}
		if (not matched) {
			throw bad_arg("invalid flag segment: \"{}\"", mode_str);
		}
	}

	return flags;
}

grid_cfg::grid_cfg (u8 nd, py::dict cfg) {
	u8 md=1; for (auto i{0}; i<nd; ++i) md*=3;
	
	if (py::len(cfg["step"]) != nd) {
		throw bad_arg("len(step) = {} != nd({})", py::len(cfg["step"]), nd);
	}
	for (auto ds : cfg["step"]) {
		step.push_back(py::cast<f32>(ds));
	}
	
	// setup axes
	if (py::len(cfg["axes"]) != nd) {
		throw bad_arg("len(axes) = {} != nd({})", py::len(cfg["axes"]), nd);
	}
	size_t lctr_size{1};
	for (auto axis : cfg["axes"]) {
		if (py::len(axis) <= 1) {
			throw bad_arg("len(axix)={} <= 1", py::len(axis));
		}
		
		std::vector<u32> axpts;
		std::vector<f32> edpts;
		for (auto pt : axis) {
			axpts.push_back(py::cast<u32>(pt));
			edpts.push_back(py::cast<f32>(pt) * step[shape.size()]);
		}
		
		lctr_size *= axpts.size()+1;
		shape.push_back(axpts.size()-1);
		units.push_back(axpts[axpts.size()-1]);
		axes .push_back(std::move(axpts));
		edges.push_back(std::move(edpts));
	}
	lctr.resize(lctr_size, 0);
	
	// setup flags
	if (cfg.contains("flags")) {
		this->flags = parse_grid_flags(py::cast<std::string>(cfg["flags"]).c_str());
		if (this->flags & AXIS_FLAG::CYLINDER and nd != 2) {
			throw bad_arg("{}D grid can not be used with CYLINDER flag!", nd);
		}
	}
	
	// setup nodes
	if (0 == py::len(cfg["nodes"])) {
		throw bad_arg("empty nodes list");
	}
	for (auto node : cfg["nodes"]) try {
		
		std::vector<u32> map; map.reserve(nd);
		std::vector<u32> lnk; lnk.reserve(md);
		std::vector<u8 > mcache;
		
		/*
		for (auto x : node["map"]) {
			map.push_back(py::cast<u32>(x));
		}
		*/
		if (py::len(node) != nd) {
			throw bad_arg("invalid numer of indexes {} != {}", py::len(node), nd);
		} for (auto m : node) {
			map.push_back(py::cast<u32>(m));
		}

		size_t k{0}, sh{1};
		for (int i{nd-1}; i >= 0; --i) {
			if (map[i] >= shape[i]) throw bad_arg("node is out of domain");
			
			k  += (map[i]+1)*sh;
			sh *= shape[i]+2;
		}
		if (lctr[k] != 0) throw bad_arg("node is duplicated");
		lctr[k] = nodes.size()+1;
		
		u64 mshift{0};
		if (cfg.contains("mask") and not cfg["mask"].is_none()) {
			auto info = (py::cast<py::array_t<u8, py::array::c_style>>(cfg["mask"])).request();

			auto fn = [&] (auto && fn, ssize_t n=0, ssize_t shift=0) -> void {
				if (n == nd) {
					u8 var{static_cast<u8*>(info.ptr)[shift]};
					mcache.push_back(var);
				} else for (size_t j1{axes[n][map[n]]}, j2{axes[n][1+map[n]]}; j1<j2; ++j1) {
					fn(fn, n+1, shift + j1*info.strides[n]/info.itemsize);
				}
			}; fn(fn);
			
			/* skip the node if it is fully masked */
			if (std::all_of(mcache.begin(), mcache.end(), [] (u8 m) {return m>0;})) {
				continue;
			}
			/* copy mask array */
			if (std::any_of(mcache.begin(), mcache.end(), [] (u8 m) {return m>0;})) {
				mshift = 1+mask.size();
				std::copy(mcache.begin(), mcache.end(), std::back_inserter(mask));
			}
		}
		nodes.push_back({std::move(map), std::move(lnk), mshift});
	
	} catch (std::exception &e) {
		throw bad_arg("node[{}]: {}", nodes.size(), e.what());
	}
	
	// build links
	for (auto& node : nodes) {
		auto &&fn = [&, k=0] (auto &&fn, u8 n, u32 sh=0, u32 w=1) mutable -> void {
			if (n>0) for (u32 i{1}, m; i<=3; ++i) {
				m=node.map[n-1]+i%3;
				if (CHECK_BIT(flags, n) and m%(1+shape[n-1]) == 0) {
					m = (m != 0)? 1 : shape[n-1];
				}
				fn(fn, n-1, sh+w*m, w*(2+shape[n-1]));
			} else {
				if (k) {
					node.lnk.push_back(lctr[sh]);
				} k++;
			}
		}; fn(fn, nd);
		py::print(node.lnk);
	}

}
