#include <ranges>

/******************************************************************************/
u16  add_constant(csection_set_cfg *cfg, f32 arg) {
	auto it = std::find (cfg->consts.begin(), cfg->consts.end(), arg);
	if (it != cfg->consts.end()) {
		return u16(it - cfg->consts.begin());
	} else {
		cfg->consts.push_back(arg);
		return u16(cfg->consts.size() - 1);
	}
}

/******************************************************************************/
void add_particle(csection_set_cfg* cfg, py::dict entry) {
	f32         encfft{0.0};
	std::string name;
	
	for (const auto& [key, val] : entry) switch (_hash(key)) {
		default:
			throw bad_arg("PARTICLE: unknown field \"{}\"!"
			, py::cast<std::string>(key));
		case "TYPE"_hash:
			continue;
		case "KEY"_hash:
			name = py::cast<std::string>(val);
			continue;
		case "ENCFFT"_hash:
			encfft = py::cast<f32>(val);
			continue;
	}
	
	if (name.empty()) {
		throw bad_arg("PARTICLE: \"KEY\" is not defined!");
	}
	
	/* check for duplicate */
	for (const auto& other : cfg->pt_list) {
		if (name == other) {
			throw bad_arg\
			("PARTICLE blocks for \"KEY\":\"{}\" already defined!",name);
		}
	}
	
	cfg->pt_list.push_back(name);
	cfg->consts.push_back(encfft);
	logger::debug("add PARTICLE \"{}\"", name);
};

/******************************************************************************/
db_group_t::db_group_t (const csection_set_cfg* cfg, py::dict entry) {
	
	flags.reset();
	
	/* parse vals */
	for (auto [key, val] : entry) switch (_hash(key)) {
		default:
			throw bad_arg
			("BACKGROUND: unknown field \"{}\"!", py::cast<std::string>(key));
		case "TYPE"_hash:
			continue;
		case "KEY"_hash:
			flags.set(DESCR_DEF);
			bgkey = py::cast<std::string>(entry["KEY"]);
			descr = fmt::format("{} + {}", cfg->pt_list.back(), bgkey);
			continue;
		case "MASSRATE"_hash:
			flags.set(MRATE_DEF);
			massrate = py::cast<f32>(entry["MASSRATE"]);
			continue;
	}
	
	if (not flags[DESCR_DEF]) {
		throw bad_arg("BACKGROUND: \"KEY\" is not defined!");
	}
	if (not flags[MRATE_DEF]) {
		massrate = 0.0f;
		logger::warning("BACKGROUND: \"MASSRATE\" is not defined!");
	}

	/* set particle */
	pt_index = cfg->pt_list.size()-1;

	/* set background */
	bg_index = std::ranges::find(cfg->bg_list, bgkey) - cfg->bg_list.begin();

	/* set channels (the second one will be updated) */
	ch_index[0] = cfg->db_entries.size();
	ch_index[1] = cfg->db_entries.size();
};

/******************************************************************************/
//~ void add_db_entry(csection_set_cfg* cfg, py::dict entry, py::dict opts) {
	//~ if (cfg->db_groups.empty()) {
		//~ throw bad_arg("\"BACKGROUND\" block is not defined yet!");
	//~ }
	//~ cfg->db_entries.emplace_back(cfg, entry, opts);
	//~ cfg->db_groups.back().ch_index[1] = cfg->db_entries.size();
//~ };

db_entry_t::db_entry_t
(const csection_set_cfg *cfg, py::dict entry, py::dict opts) {
	
	flags.reset();

	/* parse fields */
	for (auto [key, val] : entry) switch (_hash(key)) {
		
		default:
			throw bad_arg("unknown field \"{}\"!", py::cast<std::string>(key));
		case "TYPE"_hash:
			descr = py::cast<std::string>(val);
			switch (_hash(val)) {
				default:
					throw bad_arg("invalid \"TYPE\" field: \"{}\"!", descr);
				case "EFFECTIVE"_hash:
					throw bad_arg("\"EFFECTIVE\" cross-sections aren't supported, yet!");
				case "EXCHANGE"_hash:
					throw bad_arg("\"EXCHANGE\" cross-sectionns aren't supported, yet!");
				case "ELASTIC"_hash:
					opc = opcode::ELASTIC;
					continue;
				case "EXCITATION"_hash:
					opc = opcode::EXCITATION;
					continue;
				case "VIBRATIONAL"_hash:
					opc = opcode::VIBRATIONAL;
					continue;
				case "ROTATIONAL"_hash:
					opc = opcode::ROTATIONAL;
					continue;
				case "DISSOCIATION"_hash:
					opc = opcode::DISSOCIATION;
					continue;
				case "IONIZATION"_hash:
					opc = opcode::IONIZATION;
					continue;
				case "ATTACHMENT"_hash:
					opc = opcode::ATTACHMENT;
					continue;
			}
			continue;
		case "THRESHOLD"_hash:
			enth = py::cast<f32>(entry["THRESHOLD"]);
			flags.set(ENTH_DEF);
			continue; 
		case "CSEC"_hash:
			flags.set(CSEC_DEF);
			continue; 
		case "MTCS"_hash:
			flags.set(MTCS_DEF);
			continue; 
		case "DCSFN"_hash:
			flags.set(DCSFN_DEF);
			continue; 
		case "OPBPARAM"_hash:
			extra["OPBPARAM"] = py::cast<f32>(entry["OPBPARAM"]);
			flags.set(OPBPARAM_DEF);
			continue;
		case "PRODUCTS"_hash:
			extra["products"] = py::cast<std::string>(entry["PRODUCTS"]);
			flags.set(PRODUCTS_DEF);
			continue;
		case "TRANSFORM"_hash:
			throw bad_arg("\"TRANSFORM\" is not implemented yet!");
		case "COMMENT"_hash: try {
				extra["comment"] = py::cast<std::string>(entry["COMMENT"]);
			} catch (...) {}
			continue;
		case "REF"_hash: try {
				extra["ref"] = py::cast<std::string>(entry["REF"]);
			} catch (...) {}
			continue;
	}
	
	/* set energy threshold */
	if (not flags[ENTH_DEF]) {
		enth = 0.0f;
	}
	
	/* check ste stuff */
	if (enth < 0.0f \
	or (enth != 0.0f and opc == opcode::ELASTIC) \
	or (enth == 0.0f and opc != opcode::ELASTIC and opc != opcode::ATTACHMENT)) {
		throw bad_arg("{}/THRESHOLD = {} makes no sense!", descr, enth);
	}
	/****************************************************************************/
	if (opc >= opcode::IONIZATION and (flags[MTCS_DEF] or flags[DCSFN_DEF])) {
		throw bad_arg("{}/MTCS makes no sense!", descr);
	}
	if (opc != opcode::IONIZATION and flags[OPBPARAM_DEF]) {
		throw bad_arg("{}/OPBPARAM makes no sense!", descr);
	}
	/****************************************************************************/
	if (flags[CSEC_DEF]) {
		fns["CS0"] = read_csect(entry["CSEC"], enth, py::dict{**opts});
	}
	/****************************************************************************/
	if (flags[MTCS_DEF]) {
		if (not (flags[CSEC_DEF] or flags[DCSFN_DEF])) {
			throw bad_arg("\"MTCS\" & [\"CSEC\" or \"DCSFN\"] should be defined!");
		}
		fns["CS1"] = read_csect(entry["MTCS"], enth, py::dict{**opts});
	}
	/****************************************************************************/
	if (flags[DCSFN_DEF]) {
		if (flags[CSEC_DEF] and flags[MTCS_DEF]) {
			throw bad_arg("can not use all three of: \"CSEC\" & \"MTCS\" & \"DCSFN\"!");
		}
		fns["DCS"] = \
		[enth=enth, fn=py::cast<py::function>(entry["DCSFN"])] (f32 enel) -> f32 {
			return enel >= enth ? py::cast<f32>(fn(enel-enth)) : NAN;
		};
	}
	/****************************************************************************/
	if (not (flags[CSEC_DEF] or flags[MTCS_DEF] or flags[DCSFN_DEF])) {
		throw bad_arg("cross-section is not defined!");
	}
	/****************************************************************************/
	if (flags[CSEC_DEF] and flags[MTCS_DEF]) {
		fns["DCS"] = \
		[enth=enth, fn0=fns["CS0"], fn1=fns["CS1"]] (f32 enel) -> f32 {
			if (enel >= enth) {
				f32 s0{fn0(enel)};
				f32 s1{fn1(enel)};
				f32 ef{enth > 0 ? enel/enth : 0.0f};
				return DCS_from_MTCS(s1, s0, ef);
			} else return NAN;
		};
	}
	/****************************************************************************/
	if (flags[CSEC_DEF] and flags[DCSFN_DEF]) {
		fns["CS1"] = \
		[enth=enth, fn0=fns["CS0"], fnX=fns["DCS"]] (f32 enel) -> f32 {
			if (enel >= enth) {
				f32 s0{fn0(enel)};
				f32 xi{fnX(enel)};
				f32 ef{enth > 0 ? enel/enth : 0.0f};
				return s0*MTCS_from_DCS(xi, ef);
			} else return NAN;
		};
	}
	/****************************************************************************/
	if (flags[MTCS_DEF] and flags[DCSFN_DEF]) {
		fns["CS0"] = \
		[enth=enth, fn1=fns["CS1"], fnX=fns["DCS"]] (f32 enel) -> f32 {
			if (enel >= enth) {
				f32 s1{fn1(enel)};
				f32 xi{fnX(enel)};
				f32 ef{enth > 0 ? enel/enth : 0.0f};
				return s1/MTCS_from_DCS(xi, ef);
			} else return NAN;
		};
	}
	
	/****************************************************************************/
	logger::debug("add CHANNEL \"{}, {}{}\" {}"
	, cfg->db_groups.back().descr, this->descr
	, fns.contains("DCS") ? "+DCS" : ""
	, flags.to_string('-','*')
	);
}

inline
f32 from_table(const f32 tab[], u16 size, f32 arg) {
	arg = arg + 0.0625f;
	i32 k; f32 m{2*frexpf(FLT_EPSILON+arg*arg, &k)};
	switch ((k+8>0)*0x1 | (k+8>=size)*0x2) {
		case 0x0:
			return 0.0f;
		case 0x1:
			return tab[k+7]*(2.0f-m) + tab[k+8]*(m-1.0f);
		default:
			return NAN;
	}
}

struct table_t {
	f32 *_table;
	u16 _table_size;
	u16 _n_rows;
	u16 _n_entries;

	inline
	f32& enth (u16 j) const {
		return _table[j];
	}
	inline
	f32& rmax (u16 j) const {
		return _table[_n_entries + j];
	}
	inline
	f32* operator [] (u16 k) const {
		return _table + _n_entries*2 + _table_size*k;
	}
	inline
	f32  operator () (u16 j, f32 arg) const {
		return from_table((*this)[j], _table_size, arg); 
	}
};

/******************************************************************************/

/* this function builds look-up table */
void build_table (csection_set_cfg *cfg, py::dict opts) {
	bool _debug = py::cast<bool>(opts.attr("get")("debug", false));
	
	/* determine look-up table's size */
	size_t n_rows{0}, n_entries{cfg->db_entries.size()};
	py::print(cfg->consts);
	py::print(cfg->pt_list);
	py::print(cfg->bg_list);
	fmt::print("GROUPS:\n");
	for (auto& group : cfg->db_groups) {
		fmt::print("\t{:<15} PT#[{:03d}] BG#[{:03d}] CH#[{:03d}, {:03d}] {}\n"
		, group.descr, group.pt_index, group.bg_index
		, group.ch_index[0], group.ch_index[1]
		, group.flags.to_string('-','*')
		);
		for (u16 k{group.ch_index[0]}, n{group.ch_index[1]}; k<n; ++k) {
			n_rows += 1 + cfg->db_entries[k].fns.contains("DCS");
			py::print(cfg->db_entries[k]);
		}
	}
	fmt::print("{} groups, {} entries, {} rows\n"
	, cfg->db_groups.size()
	, n_entries
	, n_rows
	);
	
	std::vector<f32> _table(n_entries*2 + n_rows*(cfg->tsize-1));
	
	table_t table{_table.data(), u16(cfg->tsize-1), u16(n_rows), u16(n_entries)};
	
	/* build look-up table */
	u16 j1 = cfg->db_entries.size();
	
	if (_debug) {
		py::print(fmt::format("LOOKUP-TABLE ROW SIZE = {}", cfg->tsize-1));
	}
	for (auto& group : cfg->db_groups) {
		f64 r0, rmx{0.0f}, rsh{0.0f};
		f32 s0, s1, xi, enel, enth, cfft;
		for (u16 j{group.ch_index[0]}, n{group.ch_index[1]}; j<n; ++j) {
			auto& entry{cfg->db_entries[j]};
			
			bool containsDCS{entry.fns.contains("DCS")};
			
			if (_debug) {
				py::print(fmt::format("BUILDING LOOKUP-TABLE #{:03d} for \"{}, {}\":"
				, j, group.descr, entry.descr));
			}
			if (_debug and containsDCS) {
				py::print(fmt::format("|{:>10}|{:>10}|{:>10}|{:>10}|{:>8}|"
				, "energy", "c_rate", "σ", "σₘ", "DCS"));
				py::print(54*"-"s);
			}
			if (_debug and not containsDCS) {
				py::print(fmt::format("|{:>10}|{:>10}|{:>10}|"
				, "energy", "c_rate", "σ"));
				py::print(34*"-"s);
			}
			
			enth = entry.enth;
			cfft = cfg->cffts[group.pt_index];
			for (u8 k{0}; k<cfg->tsize-1; ++k) {
				enel = cfg->points[k] + enth;
				if (enel == 0) { /* fix infinity */
					enel = 0.5f*cfg->points[1];
				}
				
				/* write cross-section */
				s0 = entry.fns.at("CS0")(enel);
				if (isfinite(s0) and s0 > 0.0f) {
					r0  = rsh + f64(s0 * sqrtf(enel/cfft));
					rmx = std::max(rmx, r0);
				} else {
					r0 = rsh; // no-collision fallback
				}
				table[j][k] = r0;
				
				/* write DCS */
				if (containsDCS) {
					s1 = entry.fns.at("CS1")(enel);
					xi = entry.fns.at("DCS")(enel);
					if (fabsf(xi) <= 1.0) {
						// ok;
					} else if (r0 == rsh) {
						xi = 0.0f; // no-collision fallback
					} else {
						throw std::logic_error \
						(fmt::format("invalid DCS value ({}) at {} eV", xi, enel));
					}
					table[j1][k] = xi;
				}
				
				if (_debug and containsDCS) {
					py::print(fmt::format("|{:>10.3e}|{:>10.3e}|{:>10.3e}|{:>10.3e}|{:>8.4f}|"
					, enel, r0-rsh, s0, s1, xi));
				}
				if (_debug and not containsDCS) {
					py::print(fmt::format("|{:>10.3e}|{:>10.3e}|{:>10.3e}|"
					, enel, r0-rsh, s0));
				}
			}
			if (_debug) {
				py::print((containsDCS ? 54 : 34)*"-"s);
			}
			j1 += containsDCS;
			/* final check */
			if (rmx == rsh) {
				throw bad_arg
				("CHANNEL#{:03d}: collision with zero effect!", j);
			} else {
				rsh = rmx;
			}
			/* update entry.RATE \& other stuff */
			entry.fns["C_RATE"] = [tab=std::vector(table[j], table[j+1])\
			, size=cfg->tsize-1, enth] (f32 arg) -> f32 {
				return from_table(tab.data(), size, arg-enth);
			};
			table.enth(j) = enth;
			table.rmax(j) = rmx; 
			entry.rmax    = rmx;
		}
	}
	
	//~ for (auto& t : _table)
		//~ py::print(t);
		
};
