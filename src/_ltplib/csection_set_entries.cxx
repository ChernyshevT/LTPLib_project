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
db_group_t::db_group_t (csection_set_cfg* cfg, py::dict entry) {
	
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
