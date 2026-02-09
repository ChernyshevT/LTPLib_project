#include <set>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <ranges>
#include <string_view>

#include "api_backend.hxx"
#include "def_csection_set.hxx"
#include "csection_set_utils.cxx"
#include "csection_set_entries.cxx"

/******************************************************************************/

auto to_string_view = [] (auto &&r) -> std::string_view {
return std::string_view(r.begin(), std::ranges::distance(r));
};

auto not_empty = [](auto x) -> bool {
	return not x.empty();
};

std::map<std::string, group_flags_t> parse_bg_flags(py::handle str) {
	
	using std::views::split, std::views::filter, std::views::transform, std::views::drop;
	

	std::map<std::string, group_flags_t> bg_flags;
	for (auto row : str.cast<std::string_view>()
	              | split(' ')
	              | filter(not_empty)
	              | transform([] (auto&& entry) {
	                	return entry | split(':') | transform(to_string_view);
	                })
	) {
	
		auto key = std::string(row.front());
		if (not key.empty()) {
			bg_flags[key] = 0;
		} else {
			throw bad_arg("");
		}

		for (auto flag : row | drop(1) | transform(to_string_view)) {
			switch (_hash(flag)) {
				default:
					throw bad_arg("");
				case "VTERM"_hash:
					bg_flags[key].set(VTERM_DEF);
					continue;
				case "VFLUX"_hash:
					bg_flags[key].set(VFLUX_DEF);
					continue;
			}
		}
	}
	
	for (auto [k,v] : bg_flags) {
		std::cout<<std::format("{:<10} {}\n", k, v.to_string());
	}

	return bg_flags;
}

/******************************************************************************/
csection_set_cfg::csection_set_cfg (
	std::vector<py::dict> entries,
	f32 max_energy_i,
	py::str ptinfo_i,
	py::str bginfo_i,
	py::dict opts
) :
	ptinfo(py::cast<decltype(ptinfo)>(ptinfo_i.attr("split")())),
	max_energy(max_energy_i)
{
	std::vector<f64> ptabs[3];
	decltype(progs)     pprogs[2];
	
	std::set<std::string> ptset, bgset;
	
	this->db_groups.reserve(entries.size());
	this->db_entries.reserve(entries.size());
	
	if (opts.contains("bginfo")) {
		this->bg_flags = parse_bg_flags(opts["bginfo"]);
	}
	
	/****************************************************************************/
	if (max_energy < 0.25f) {
		throw bad_arg("invalid max_energy ({:e} < 0.25 eV)!", max_energy);
	} else {
		points.reserve(256);
		f32 e0;
		int k{0}; do {
			e0 = ldexpf((k%2 ? 1.0f/sqrtf(2.0f) : 0.5f), k/2-3) - 0.0625f; k += 1;
			points.push_back(e0);
		} while (e0 <= max_energy*1.5);
		tsize = k+1;
	}
	/****************************************************************************/
	if (not ptinfo.size()) {
		throw bad_arg("empty \"ptinfo\" parameter!");
	}
	
	cffts.resize(ptinfo.size());
	ntype  = cffts.size();
	/****************************************************************************/
	struct {
		u16 jset;
		
		u8  skip=0;
		u8  n0_def=0;
		u8  n0_idx=0;
		u8  t0_def=0;
		u8  v0_def=0;
		u8  massrate_def=0;
		u8  massrate_idx=0;
		void reset () {
			skip   = 0;
			n0_def = 0;
			n0_idx = 0;
			t0_def = 0;
			v0_def = 0;
			massrate_def = 0;
			massrate_idx = 0;
		}
	} flags;
	/****************************************************************************/
	auto add_cs = [&, k=0] (u8 tag, bool is_first, db_entry_t &entry) mutable {
		

		f64 r0, rmx{0.0}, rsh{is_first? ptabs[0].back() : 0.0};
		f32 s0, enel, enth{entry.enth};
		
		std::vector<f32> rvec(tsize);
		for (size_t k{0}; k<tsize; ++k) {
			if (k == 0) {
				rvec[0] = enth;
				ptabs[1].push_back(enth);
				if (entry.fns.contains("DCS")) {
					ptabs[2].push_back(enth);
				}
				continue;
			}
			enel = points[k-1] + enth;
			if (enel == 0) { /* fix infinities */
				enel = 0.5f*points[1];
			}
			
			s0 = entry.fns.at("CS0")(enel);
			if (s0 > 0.0f) {
				r0  = rsh + f64(s0 * sqrtf(enel/cffts[tag]));
				rmx = std::max(rmx, r0);
			} else {
				r0 = rsh; // no-collision fallback
			}
			rvec[k] = r0;
			
			ptabs[1].push_back(r0);
			
			if (entry.fns.contains("DCS")) {
				//f32 s1{entry.fns.at("CS1")(enel)};
				f32 xi{entry.fns.at("DCS")(enel)};
				if (fabsf(xi) <= 1.0) {
					// ok;
				} else if (r0 == rsh) {
					xi = 0.0f; // no-collision fallback
				} else {
					throw std::logic_error \
					(std::format("invalid DCS value ({}) at {} eV", xi, enel));
				}
				ptabs[2].push_back(xi);
			}
		}
		if (rmx == rsh) {
			throw bad_arg("collision with zero effect!");
		} else {
			ptabs[0].push_back(rmx);
		}
		++k;
		
	}; /* end add_cs */
	
	/****************************************************************************/
	auto add_op = [&] (u8 k, opcode opc, u16 arg=0) -> u16 {
		pprogs[k].push_back(mprog_t{opc, arg});
		//logger::debug("adding cmd {}{}", MPROG_DESCR[opc], k?'*':'\0');
		return pprogs[k].size()-1;
	};
	/****************************************************************************/
	auto add_cf = [&] (f32 arg) -> u16 {
		auto it = std::find (cffts.begin()+ntype, cffts.end(), arg);
		if (it != cffts.end()) {
			return u16(it-cffts.begin());
		} else {
			cffts.push_back(arg);
			return u16(cffts.size()-1);
		}
	};
	/****************************************************************************/
	
	std::map<u16, std::tuple<std::string, u16>> einfo;
	
	//~ u16 defbg=bginfo.size();
	
	u16 DCS_NUM{0};
	
	// first pass -- parse entries into configuration sequence
	flags.jset=0;
	
	//~ bool TYPE_isdefined{false};
	
	u16 k2{0}, tag{0};
	for (const auto& entry : entries) try {

		/* select particle and background *****************************************/
		switch (_hash(entry["TYPE"])) {
			case "PARTICLE"_hash: {
				add_particle(this, entry);
				
				std::string key;
				if (entry.contains("KEY") and entry.contains("ENCFFT")) {
					key = py::cast<std::string>(entry["KEY"]);
					auto pos = std::find(ptinfo.begin(), ptinfo.end(), key);
					if (pos != ptinfo.end() and tag <= pos-ptinfo.begin()) {
						tag = pos-ptinfo.begin();
						cffts[tag] = py::cast<f32>(entry["ENCFFT"]);
						//TYPE_isdefined = true;
					} else {
						throw bad_arg("invalid order of active components (\"{}\")!", key);
					}
				}	else {
					throw bad_arg ("PARTICLE:  \"KEY\" or/and \"ENCFFT\" is/are not defined!");
				}
			} continue;
			
			case "BACKGROUND"_hash:
				if (pt_list.empty()) {
					throw bad_arg ("\"PARTICLE\" block is not defined yet!");
				} else {
					flags.reset(); //TODO REMOVE
					
					db_group_t db_group(this, entry);
					/* skip the group if it is not active */
					if (not (bg_flags.contains(db_group.bgkey) or bg_flags.contains("*"))) {
						logger::debug("skip GROUP \"{}\"", db_group.descr);
						flags.skip = true;
						continue;
					}
					/* register the background if it is the new one */
					if (db_group.bg_index == bg_list.size()) {
						bg_list.push_back(db_group.bgkey);
					}
					if (bg_flags.contains(db_group.bgkey)) {
						db_group.flags |= bg_flags.at(db_group.bgkey);
					}
					if (bg_flags.contains("*")) {
						db_group.flags |= bg_flags.at("*");
					}
					
					/* register m/M ratio const */ //TODO REMOVE
					if (db_group.flags[MRATE_DEF]) {
						flags.massrate_def = 1;
						flags.massrate_idx = add_cf(2.0f*db_group.massrate);
						add_op(0, opcode::MASSRATE, flags.massrate_idx);
					}

					add_op(0, opcode::SELECTBG, db_group.bg_index);
					// null-collision search cmd
					if (k2) {
						pprogs[0][k2].arg = flags.jset;
					}
					flags.jset = 0;
					k2 = add_op(0, opcode::SEARCH);
					
					logger::debug("add GROUP \"{}\" {}"
					, db_group.descr
					, db_group.flags.to_string('-','*'));
					
					db_groups.push_back(std::move(db_group));
				}
				continue;
		
			default:
				if (flags.skip) continue;
		}
		
		/* add processess *********************************************************/
		if (db_groups.empty()) {
			throw bad_arg("\"BACKGROUND\" block is not defined yet!");
		}
		db_entries.emplace_back(this, entry, opts);
		db_groups.back().ch_index[1] = db_entries.size();
		
		//auto& db_group{db_groups.back()};
		auto& db_entry{db_entries.back()};
		
		chinfo.push_back(db_entry.descr);

		add_cs(tag, flags.jset>0, db_entry);
		add_op(0, db_entry.opc);
		
		if (db_entry.fns.contains("DCS")) {
			einfo[pprogs[0].size()-1] = {"DCS", pprogs[1].size()};
			add_op(1, opcode::SETDCSFACTOR, DCS_NUM++);
			add_op(1, opcode::END);
		}
		
		if (entry.contains("OPBPARAM")) {
			einfo[pprogs[0].size()-1] = {"OPB", pprogs[1].size()};
			add_op(1, opcode::SETOPBFACTOR, add_cf(py::cast<f32>(entry["OPBPARAM"])));
			add_op(1, opcode::END);
		}
		++flags.jset;
		
	} catch (const std::exception& e) {
		throw bad_arg("entry[{}]: {}", &entry-&entries[0], e.what());
	}
	
	if (k2) {
		// null-collision search cmd
		pprogs[0][k2].arg = flags.jset;
	} add_op(0, opcode::END);
	
	
	for (auto k{0}; pprogs[0][k].opc != opcode::END; k++) {
		if (einfo.contains(k)) {
			auto [descr, j] = einfo[k];
			if ("DCS"  == descr) {
				pprogs[0][k].arg  = pprogs[0].size()+j-k;
				pprogs[1][j].arg += ptabs[0].size();
			}
			if ("OPB" == descr) {
				pprogs[0][k].arg  = pprogs[0].size()+j-k;
			}
		} else if (pprogs[0][k].opc > opcode::SEARCH) {
			pprogs[0][k].arg = pprogs[0].size()-1-k;
		}
	}
	
	std::copy(pprogs[0].begin(), pprogs[0].end(), std::back_inserter(progs));
	std::copy(pprogs[1].begin(), pprogs[1].end(), std::back_inserter(progs));

	std::copy(ptabs[0].begin(), ptabs[0].end(), std::back_inserter(tabs));
	std::copy(ptabs[1].begin(), ptabs[1].end(), std::back_inserter(tabs));
	std::copy(ptabs[2].begin(), ptabs[2].end(), std::back_inserter(tabs));
	
	ncsect = ptabs[0].size(); //r_base
	nxtra  = DCS_NUM;
	
	//~ u16 k{0};
	//~ for (auto &entry : db_entries) {
		//~ entry.rmax = tabs[k];
		//~ entry.fns["C_RATE"]  = [&, fn=csection_t(&tabs[ncsect + k*tsize])] (f32 enel) {
			//~ if (enel - fn.enth <= max_energy) {
				//~ return fn[enel];
			//~ } else {
				//~ return NAN;
			//~ }
		//~ };
		//~ ++k;
	//~ }

	build_table(this, opts);
}
