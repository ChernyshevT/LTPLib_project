/*******************************************************************************
 * Make cross-section function from the set of point pairs
 ******************************************************************************/
#define DEFAULT_EXTERP 0.25
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
			throw bad_arg("point#{} unsorted arguments!", j);
		}
		
		// collect  extrapolation coeffs.
		if (x > 0.0f and exterp >= log10f(xys[n-1][0]) - log10f(x)) {
			mx  += logf(x);
			my  += logf(y);
			mxx += logf(x)*logf(x);
			mxy += logf(x)*logf(y);
			++nex;
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

		if (nex == 1) {
			throw bad_arg("can not extrapolate: no enough points!");
		}
		if (not (isfinite(a) or isfinite(b))) {
			throw bad_arg("can not extrapolate: invalid values!");
		}
		if (a > 0.0f) {
			throw bad_arg("can not extrapolate: growing function!");
		}

		logger::debug(
		"build extrapolation using {} points"
		" ({:8.2e}, {:8.2e} eV): a={:+9.2e}, b={:+9.2e}"
		, nex, xys[n-nex][0], xys[n-1][0], a, b);
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
#undef DEFAULT_EXTERP

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
			f32 val{py::cast<f32>(fn(x>0.0f? x : FLT_EPSILON, enth))};
			return scale*(val>=0? val : 0.0f);
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
		u32  n0=0, n1=0;
		// scan lines
		for (u32 n{1u}; std::getline(fp, line); n++) try {
			
			// skip
			if (n < lineno or trim(line).empty()) {
				continue;
			}
			
			// if pattern is founded in lxcat-file
			if (not (foundPattern or startScan)
			    and (line.find(pattern) != std::string::npos)) {
				foundPattern = true;
				continue;
			}
			
			// start/stop scan lines in lxcat-file
			if (foundPattern and line.starts_with("-----")) {
				if (not startScan) {
					n0 = n;
					startScan = true;
					continue;
				} else {
					n1 = n;
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
			throw bad_arg("{} LINE#{}: {}", fname, n, e.what());
		}
		
		if (not foundPattern and not pattern.empty()) {
			throw bad_arg("didn't find pattern \"{}\" in file \"{}\""\
			, pattern, fname);
		}
		
		try {
			return from_data(std::move(xys), enth, opts);
		} catch (std::exception& e) {
			throw bad_arg("{} LINES#{}--{}: {}", fname, n0, n1, e.what());
		}
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
 * Benchmark calculations for anisotropic scattering in  kinetic models for low
 * temperature plasma. doi: 10.1088/1361-6463/ad3477, cite: flynn2024]
 ******************************************************************************/

f32 MTCS_from_DCS (f32 x, f32 ef) {
	/* x{ξ}, ef{ε/εₜ or 0} -> σₘ/σ */
	if (fabsf(x) >= 1.0f) {
		throw bad_arg("invalid DCS value: |{}| >= 1!", fabsf(x));
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
	/* sm{σₘ}, s0{σ}, ef{ε/εₜ or 0} -> ξ */
	if (s0 == 0.0f or fabsf(ef-1.0f) <= FLT_EPSILON) {
		return 0.0f;
	}
	
	f64 x, xs[]{0.0, 1.0}, vm, v0{fabs(1.0-sm/s0)}, f0{1.0};
	
	if (ef != 0) {
		f0 = sqrtf(1.0-1.0/ef);
	}
	
	if (v0 >= f0) {
		throw bad_arg("invalid MTCS/ICS ratio: |1-σₘ/σ| >= √(1-ε/εₜₕ), "
		"(|1 - {:5.2e}/{:5.2e}| = {:f} >= {:f})!", sm, s0, v0, f0);
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
