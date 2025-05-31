#include <set>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include "def_csection_set.hxx"
#define  DEFAULT_EXTERP 0.25

csfunc_t read_csect (py::handle arg, f32 enth, py::dict opts);

f32 DCS_from_MTCS (f32 sm, f32 s0, f32 ef);

f32 MTCS_from_DCS (f32 x, f32 ef);

void update_cache (csection_set_cfg *cfg) {
};

enum state : u8 {
	ENTH_DEF=0,
	CSEC_DEF,
	MTCS_DEF,
	DCSFN_DEF,
	OPBPARAM_DEF,
	NONCONS,
};

/******************************************************************************/
db_entry_t::db_entry_t (py::dict entry, py::dict opts, const std::vector<std::string> &ptinfo) {
	
	enth = 0.0f;
	FLAGS.reset();

	// check fields
	for (auto [key, val] : entry) switch (_hash(key)) {
		
		default:
			throw bad_arg("unknown field \"{}\"", py::cast<std::string>(key));
		case "TYPE"_hash:
			descr = py::cast<std::string>(val);
			switch (_hash(val)) {
				default:
					throw bad_arg("invalid \"TYPE\" field: \"{}\"!", descr);
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
			FLAGS.set(ENTH_DEF);
			continue; 
		case "CSEC"_hash:
			FLAGS.set(CSEC_DEF);
			continue; 
		case "MTCS"_hash:
			FLAGS.set(MTCS_DEF);
			continue; 
		case "DCSFN"_hash:
			FLAGS.set(DCSFN_DEF);
			continue; 
		case "OPBPARAM"_hash:
			extra["OPBPARAM"] = py::cast<f32>(entry["OPBPARAM"]);
			FLAGS.set(OPBPARAM_DEF);
			continue;
		case "SPAWN"_hash:
			throw std::logic_error ("\"SPAWN\" is not implemented yet!");
		case "COMMENT"_hash: try {
				extra["comment"] = py::cast<std::string>(entry["COMMENT"]);
			} catch (...) {}
			continue;
		case "REF"_hash: try {
				extra["ref"] = py::cast<std::string>(entry["REF"]);
			} catch (...) {}
			continue;
	}
	//fmt::print("entry {}: ", info); std::cout<<FLAGS<<"\n";
	
	if (enth < 0 \
	or (enth != 0 and opc == opcode::ELASTIC) \
	or (enth == 0 and opc != opcode::ELASTIC and opc != opcode::ATTACHMENT)) {
		throw bad_arg("{}/THRESHOLD = {}", descr, enth);
	}
	/****************************************************************************/
	if (opc >= opcode::IONIZATION and (FLAGS[MTCS_DEF] | FLAGS[DCSFN_DEF])) {
		throw bad_arg("{}/MTCS makes no sense", descr);
	}
	/****************************************************************************/
	if (FLAGS[CSEC_DEF]) {
		fns["CS0"] = read_csect(entry["CSEC"], enth, py::dict{**opts});
	}
	/****************************************************************************/
	if (FLAGS[MTCS_DEF]) {
		fns["CS1"] = read_csect(entry["MTCS"], enth, py::dict{**opts});
	}
	/****************************************************************************/
	if (FLAGS[DCSFN_DEF]) {
		fns["DCS"] = \
		[enth=enth, fn=py::cast<py::function>(entry["DCSFN"])] (f32 enel) -> f32 {
			return enel >= enth ? py::cast<f32>(fn(enel-enth)) : NAN;
		};
	}
	/****************************************************************************/
	if (FLAGS[CSEC_DEF] and FLAGS[MTCS_DEF]) {
		if (FLAGS[DCSFN_DEF]) {
			throw bad_arg("can not use both \"MTCS\" & \"DCSFN\"");
		}
		
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
	if (FLAGS[CSEC_DEF] and FLAGS[DCSFN_DEF]) {
		if (FLAGS[MTCS_DEF]) {
			throw bad_arg("can not use both \"MTCS\" & \"DCSFN\"");
		}
		
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
	if (FLAGS[MTCS_DEF] and FLAGS[DCSFN_DEF]) {
		if (FLAGS[CSEC_DEF]) {
			throw bad_arg("can not use all three of \"CS\", \"MTCS\" & \"DCSFN\"");
		}
		
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
	bginfo(py::cast<decltype(ptinfo)>(bginfo_i.attr("split")())),
	max_energy(max_energy_i)
{
	std::vector<f64> ptabs[3];
	decltype(progs)     pprogs[2];
	
	std::set<std::string> ptset, bgset;
	
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
		logger::debug("adding cmd {}{}", MPROG_DESCR[opc], k?'*':'\0');
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
				if (not TYPE_isdefined) throw bad_arg
				("\"PARTICLE\" entry is not defined!");
				
				flags.reset();
				std::string key;
				if (entry.contains("KEY")) {
					key = py::cast<std::string>(entry["KEY"]);
					if (not bgset.insert(key).second) throw bad_arg("background \"{}\" is already set!", key);
					
					if (not defbg) {
						logger::debug("csection_set: add background \"{}\"", key);
						bginfo.push_back(key);
					}
					auto pos = std::find(bginfo.begin(), bginfo.end(), key);
					if (pos != bginfo.end()) {
						flags.n0_def = 1;
						flags.n0_idx = pos-bginfo.begin();
					} else {
						logger::info("csection_set: skip background \"{}\"", key);
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
		db_entries.emplace_back(entry, opts, ptinfo);
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

		// add cumulative-rate function
		//~ entry.fns["C_RATE"] = 
		//~ [rvec, fn=csection_t(&rvec[0]), max_energy=max_energy] (f32 enel) -> f32 {
			//~ if (enel-fn.enth <= max_energy) {
				//~ return fn[enel];
			//~ } else {
				//~ return NAN;
			//~ } 
		//~ };
		
		//~ py::print(rvec);

	update_cache (this);
}

/******************************************************************************/
csfunc_t from_data
(std::vector<std::array<f32,2>>&& xys, f32 enth, py::dict opts) {
	
	if (xys.size() < 3) throw bad_arg("no enough data points");

	f32 exterp = py::cast<f32>(opts.attr("get")("exterp", DEFAULT_EXTERP));
	f32 scale  = py::cast<f32>(opts.attr("get")("scale", 1.0f)) \
	           * py::cast<f32>(opts.attr("get")("rescale", 1.0f));
	size_t j{0}, n{xys.size()}, nex{0};
	
	// check data
	f32 a{0.0}, b{0.0}, mx{0.0}, my{0.0}, mxx{0.0}, mxy{0.0};
	for (auto [x, y] : xys) {
		
		if (not (std::isfinite(x) or std::isfinite(y) or x >= 0)) {
			throw bad_arg("point#{} invalid data ({}, {})!", j, x, y);
		}
		
		if ((j ? xys[j-1][0] : -INFINITY) >= x) {
			throw bad_arg("point#{} unsorted values in x-column", j);
		}
		
		// collect  extrapolation coeffs.
		if (x > 0.0f and exterp >= log10f(xys[n-1][0]) - log10f(x)) {
			mx  += logf(x);
			my  += logf(y);
			mxx += logf(x)*logf(x);
			mxy += logf(x)*logf(y);
			++nex;
			//py::print(x, y, "*");
		} else {
			//py::print(x, y);
		}
		++j;
	}
	// calculate extrapolation coeffs.
	if (exterp > 0.0f) {
		mx  = mx /f32(nex);
		my  = my /f32(nex);
		mxx = mxx/f32(nex);
		mxy = mxy/f32(nex);
		
		a = (mxy - mx*my)/(mxx - mx*mx);
		b = expf(my - a*mx);
		logger::debug\
		("build extrapolation using {} points: a={:e}, b={:e}", nex, a, b);
		
		if (nex == 1 or a > 0.0f) throw bad_arg("failed to extrapolate");
	}
	
	return [=] (f32 x) -> f32 {
		
		if (x < enth) return NAN;
		
		size_t j{0}, j1{n};
		while (j < j1) {
			size_t md{(j+j1)/2};
			if (x < xys[md][0]) {
				j1 = md;
			} else {
				j  = md+1;
			}
		}

		f32 v, c, lx, rx, ly, ry;
		if (j < n) {
			j1 = j? j : j+1; 
			
			lx = xys[j1-1][0];
			ly = xys[j1-1][1];
			rx = xys[j1][0];
			ry = xys[j1][1];

			if (j and x > 0.0f and lx > 0.0f and ly > 0.0f and ry > 0.0f) {
			// log-log interpolation
				lx = logf(lx);
				ly = logf(ly);
				rx = logf(rx);
				ry = logf(ry);
				c = (ry-ly)/(rx-lx);
				v = expf(c*logf(x) + (ry - c*rx));
			} else {
			// fallback linear interpolation
				c = (ry-ly)/(rx-lx);
				v = c*x + (ry - c*rx);
				if (v < 0.0f) v = 0.0f;
			}
		} else if (exterp > 0) {
			// log-log extrapolation
			v = expf(a*logf(x))*b;
		} else {
			v = NAN;
		}
		return v*scale;
	};
}

/******************************************************************************/
csfunc_t read_csect (py::handle obj, f32 enth, py::dict opts) {
	
	// update opts
	if (py::isinstance<py::tuple>(obj)) {
		py::dict xtra;
		std::tie(obj, xtra) = py::cast<std::tuple<py::handle, py::dict>>(obj); 
		opts.attr("update")(xtra);
	}
	
	// transform from function
	if (py::isinstance<py::function>(obj)) {
		f32 scale = py::cast<f32>(opts.attr("get")("scale", 1.0f)) \
	            * py::cast<f32>(opts.attr("get")("rescale", 1.0f));
	
		return [scale, enth, fn=py::cast<py::function>(obj)] (f32 x) -> f32 {
			return scale*py::cast<f32>(fn(x, enth));
		};
	}

	// read from file
	if (py::isinstance<py::str>(obj)) {
		std::string line, fname = py::cast<std::string>(obj);
		
		auto fp = std::ifstream(std::filesystem::path(fname));
		if (not fp.is_open()) {
			throw bad_arg("error opening file \"{}\"", fname);
		} else {
			logger::debug("reading \"{}\"", fname);
		}
		
		std::vector<std::array<f32, 2>> xys;
		auto pattern = py::cast<std::string>(opts.attr("get")("search", ""));
		auto lineno  = py::cast<u32>(opts.attr("get")("lineno", 0));
		bool foundPattern = pattern.empty();
		bool startScan    = pattern.empty();
		// scan lines
		for (u32 n{1u}, n1{lineno}; std::getline(fp, line); n++) try {
			
			// skip
			if (n < n1 or trim(line).empty()) {
				continue;
			}
			
			// if pattern is founded in lxcat-file
			if (not (foundPattern or startScan) and line.starts_with(pattern)) {
				foundPattern = true;
				continue;
			}
			
			// start/stop scan lines in lxcat-file
			if (foundPattern and line.starts_with("-----")) {
				if (not startScan) {
					startScan = true;
					continue;
				}
				break; // stop scan
			}
			
			// skip if comment
			if (startScan and (line.starts_with("!")
			or line.starts_with("#")
			or line.starts_with("%")
			or line.starts_with("//")
			)) {
				continue;
			}
			
			//parse lines
			if (startScan) {
				auto [x, y] = parse_vals<f32, f32>(line);
				// check values
				if (not (std::isfinite(x) or std::isfinite(y) or x >= 0)) {
					throw bad_arg("invalid data-point ({}, {})!", x, y);
				}
				// check sorted
				if (xys.size() > 0 and xys[xys.size()-1][0] >= x) {
					throw bad_arg("unsorted values in energy-column, ({} >= {})!"\
					, xys[xys.size()-1][0], x);
				}
				xys.push_back({x, y});
				continue;
			}

		} catch (std::exception& e) {
			throw bad_arg("{} LINE#{}: {}"
			, fname, n, e.what());
		}
		
		if (not foundPattern and not pattern.empty()) {
			throw bad_arg("didn't find pattern \"{}\" in file \"{}\""\
			, pattern, fname);
		}
		return from_data(std::move(xys), enth, opts);
	}

	// read from list of pairs, numpy array , etc.
	if (py::isinstance<py::iterable>(obj)) {
		std::vector<std::array<f32, 2>> xys;
		for (auto xy : obj) {
			xys.push_back(py::cast<std::array<f32, 2>>(xy));
		}
		return from_data(std::move(xys), enth, opts);
	}
	
	throw bad_arg("invalid entry type {}"\
	, py::cast<std::string>(py::type::of(obj).attr("__name__")));
}

/******************************************************************************* 
 * Method to obtain anosotropic parameter $\xi$ from MTCS/TCS ratio, see
 * [M Flynn et al 2024 J. Phys. D: Appl. Phys. 57 255204.
 * Benchmark calculations for anisotropic scattering in  kinetic models for
 * low temperature plasma. doi: 10.1088/1361-6463/ad3477]
 ******************************************************************************/
f32 MTCS_from_DCS (f32 x, f32 ef) {
	if (fabsf(x) >= 1.0f) {
		throw bad_arg("invalid DCS value |{}| >= 1", fabsf(x));
	}
	
	f32 f0, v0;
	if (fabsf(x) >= FLT_EPSILON) {
		f0 = (ef != 0) ? sqrtf(1.0-1.0/ef) : 1.0;
		v0 = f0*(1.0 - (0.5*(1.0+x) * (log(1.0+x)-log(1.0-x)) - x) * (1.0-x)/x/x);
	} else {
		v0 = 0.0f;
	};
	return 1.0f - v0;
}

f32 DCS_from_MTCS (f32 sm, f32 s0, f32 ef) {

	if (s0 == 0.0f or fabsf(ef-1.0f) <= FLT_EPSILON) {
		return 0.0f;
	}
	
	f64 x, xs[]{0.0, 1.0}, vm, v0{fabs(1.0-sm/s0)}, f0{1.0};
	
	if (ef != 0) {
		f0 = sqrtf(1.0-1.0/ef);
	}
	
	if (v0 >= f0) {
		throw bad_arg(\
		"invalid MTCS/ICS ratio: |1-σₘ/σ| >= √(1-ε/εₜ)"
		", (1 - {:5.2e}/{:5.2e} = {:f} >= {:f})!", sm, s0, v0, f0
		);
	}
	
	do {
		x = 0.5*(xs[0]+xs[1]);
		if (x >= FLT_EPSILON) {
			// (x -> 1, vm -> 1)
			vm = f0*(1.0 - (0.5*(1.0+x) * (log(1.0+x)-log(1.0-x)) - x) * (1.0-x)/x/x);
		} else {
			// (x -> 0, vm -> 0)
			vm  = 0.0;
		}
		xs[v0 < vm] = x;
	} while (fabs(vm-v0) > FLT_EPSILON);
	
	return copysignf(x, s0-sm);
}
