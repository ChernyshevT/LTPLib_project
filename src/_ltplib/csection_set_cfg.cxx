#include <set>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <ranges>

#include "api_backend.hxx"
#include "def_csection_set.hxx"
#include "csection_set_utils.cxx"
#include "csection_set_entries.cxx"



void update_cfg (csection_set_cfg *cfg, py::dict opts) {
	
	//auto _debug = py::cast<bool>(opts.attr("get")("debug", false));
	
	if (true) {
		py::print(cfg->consts);
		py::print(cfg->pt_list);
		py::print(cfg->bg_list);
		fmt::print("GROUPS:\n");
		for (const auto& group : cfg->db_groups) {
			fmt::print("\t{:<15} PT#[{:03d}] BG#[{:03d}] CH#[{:03d}, {:03d}]\n"
			, group.descr, group.pt_index, group.bg_index
			, group.ch_index[0], group.ch_index[1]);
		}
	}
};

/******************************************************************************/
csection_set_cfg::csection_set_cfg (
	std::vector<py::dict> entries,
	f32 max_energy_i,
	py::str ptinfo_i,
	py::str bginfo_i,
	py::dict opts
) :
	ptinfo(py::cast<decltype(ptinfo)>(ptinfo_i.attr("split")())),
	bginfo(py::cast<decltype(ptinfo)>(bginfo_i.attr("split")())),
	max_energy(max_energy_i)
{
	std::vector<f64> ptabs[3];
	decltype(progs)     pprogs[2];
	
	std::set<std::string> ptset, bgset;
	
	this->db_groups.reserve(entries.size());
	this->db_entries.reserve(entries.size());
	
	/****************************************************************************/
	if (max_energy < 10.0f) {
		throw bad_arg("invalid max_energy ({:e} < 10 eV)!", max_energy);
	} else {
		points.reserve(256);
		f32 e0;
		int k{0}; do {
			e0 = ldexpf((k%2 ? 1.0f/sqrtf(2.0f) : 0.5f), k/2-3) - 0.0625f; k += 1;
			points.push_back(e0);
		} while (e0 <= max_energy);
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
		auto _debug = py::cast<bool>(opts.attr("get")("debug", false));

		if (_debug) {
			py::print(fmt::format("BUILDING LOOKUP-TABLE #{:03d} for {}"
			, k, MPROG_DESCR[entry.opc]));
			if (entry.fns.contains("DCS")) {
				py::print(fmt::format("|{:>10}|{:>10}|{:>10}|{:>10}|{:>8}|"
				, "energy", "c_rate", "σ", "σₘ", "DCS"));
				py::print(54*"-"s);
			} else {
				py::print(fmt::format("|{:>10}|{:>10}|{:>10}|"
				, "energy", "c_rate", "σ"));
				py::print(34*"-"s);
			}
		}
		
		f64 r0, rmx{0.0}, rsh{is_first? ptabs[0].back() : 0.0};
		f32 s0, s1, xi, enel, enth{entry.enth};
		
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
			s1 = s0;
			xi = 0;
			if (s0 > 0.0f) {
				r0  = rsh + f64(s0 * sqrtf(enel/cffts[tag]));
				rmx = std::max(rmx, r0);
			} else {
				r0 = rsh; // no-collision fallback
			}
			rvec[k] = r0;
			
			ptabs[1].push_back(r0);
			
			if (entry.fns.contains("DCS")) {
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
				ptabs[2].push_back(xi);
			}
			if (_debug and  entry.fns.contains("DCS")) {
				py::print(fmt::format("|{:>10.3e}|{:>10.3e}|{:>10.3e}|{:>10.3e}|{:>8.4f}|"
				, enel, r0-rsh, s0, s1, xi));
			}
			if (_debug and !entry.fns.contains("DCS")) {
				py::print(fmt::format("|{:>10.3e}|{:>10.3e}|{:>10.3e}|"
				, enel, r0-rsh, s0));
			}
		}
		if (_debug and  entry.fns.contains("DCS")) {
			py::print(54*"-"s);
		}
		if (_debug and !entry.fns.contains("DCS")) {
			py::print(34*"-"s);
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
	
	u16 defbg=bginfo.size();
	
	u16 DCS_NUM{0};
	
	// first pass -- parse entries into configuration sequence
	flags.jset=0;
	
	bool TYPE_isdefined{false};
	
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
						TYPE_isdefined = true;
					} else {
						throw bad_arg("invalid order of active components (\"{}\")!", key);
					}
				}	else {
					throw bad_arg ("PARTICLE:  \"KEY\" or/and \"ENCFFT\" is/are not defined!");
				}
			} continue;
			
			case "BACKGROUND"_hash: {
				add_db_group(this, entry);
				
				if (not TYPE_isdefined) throw bad_arg
				("\"PARTICLE\" entry is not defined!");
				
				flags.reset();
				std::string key;
				if (entry.contains("KEY")) {
					key = py::cast<std::string>(entry["KEY"]);
					//if (not bgset.insert(key).second) throw bad_arg("background \"{}\" is already set!", key);
					
					if (not defbg) {
						//logger::debug("csection_set: add background \"{}\"", key);
						bginfo.push_back(key);
					}
					auto pos = std::find(bginfo.begin(), bginfo.end(), key);
					if (pos != bginfo.end()) {
						flags.n0_def = 1;
						flags.n0_idx = pos-bginfo.begin();
					} else {
						//logger::info("csection_set: skip background \"{}\"", key);
						flags.skip = 1;
					}
				} else throw bad_arg("BACKGROUND: \"KEY\" is not defined!");

				if (not flags.skip and entry.contains("MASSRATE")) {
					flags.massrate_def = 1;
					f32 mrate{py::cast<f32>(entry["MASSRATE"])};
					if (mrate > 0.0f and mrate < 1.0f) {
						flags.massrate_idx = add_cf(2.0f*mrate);
						add_op(0, opcode::MASSRATE, flags.massrate_idx);
					} else {
						throw bad_arg("\"MASSRATE\": {}!", mrate);
					}
				}

				if (not flags.skip) {
					add_op(0, opcode::SELECTBG, flags.n0_idx);
					// null-collision search cmd
					if (k2) {
						pprogs[0][k2].arg = flags.jset;
					}
					flags.jset = 0;
					k2 = add_op(0, opcode::SEARCH); 
				}
			} continue;
		
			default: if (flags.skip) continue;
		}
		
		/* add processess *********************************************************/
		if (not flags.n0_def) {
			throw bad_arg("background is not defined yet!");
		}
		add_db_entry(this, entry, opts);
		//db_entries.emplace_back(this, entry, opts);
		auto& db_entry{db_entries.back()};
		
		chinfo.push_back\
		(fmt::format("{}+{} {}", ptinfo[tag], bginfo[flags.n0_idx], db_entry.descr));

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
	
	u16 k{0};
	for (auto &entry : db_entries) {
		entry.rmax = tabs[k];
		entry.fns["C_RATE"]  = [&, fn=csection_t(&tabs[ncsect + k*tsize])] (f32 enel) {
			if (enel - fn.enth <= max_energy) {
				return fn[enel];
			} else {
				return NAN;
			}
		};
		++k;
	}

	update_cfg(this, opts);
}
