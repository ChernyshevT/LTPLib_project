#pragma once
#include "typedefs.hxx"

//~ #define BGFLAGV0 (1<<11)

//@LISTING{start:mprog_t}
struct mprog_t {
	//op-code
	enum opc_t : u16 {
		JMP = 0,       // jump to position
		SETCSID,       // set csection id (if nonzero)
		MASSRATE,      
		SELECTBG,      // set bg's density id, [$m/M$, $u_0$, $v_{T0}$]
		SEARCH,        // start binary search for null-collision
		 ELASTIC, 
		 EXCITATION,  
		 VIBRATIONAL,
		 ROTATIONAL,
		 DISSOCIATION,
		 IONIZATION,
		 ATTACHMENT,
		SETDCSFACTOR,
		SETOPBFACTOR,
		END,
	} opc : 5; //32 commands max
	//argument
	u16
	  arg : 11;
};
typedef mprog_t::opc_t opcode;
//@LISTING{end:mprog_t}

inline const char *MPROG_DESCR[] = {
	"JMP",
	"SETCSID",
	"MASSRATE",
	"SELECTBG",
	"SEARCH",
	 "ELASTIC",
	 "EXCITATION",
	 "VIBRATIONAL",
	 "ROTATIONAL",
	 "DISSOCIATION",
	 "IONIZATION",
	 "ATTACHMENT",
	"SETDCSFACTOR",
	"SETOPBFACTOR",
	"END",
};

//@LISTING{start:csection_t}
struct csection_t {
// Cross-section for MCC-simulation (N points, exponential grid)
// Energies lies on a $2^{j/2-4}-0.0625$ grid, where $j\in\mathbb{N}$
// arg = $\varepsilon >= \varepsilon^{*}$ 
	
	const f32 enth, *tab;

	 csection_t (const f32 lnk[]) : enth{lnk[0]}, tab{lnk+1} {
	};

	inline f32 operator [] (f32 arg) const {
		arg = (arg-enth) + 0.0625f;
		int k; f32 m{2*frexpf(FLT_EPSILON+arg*arg, &k)};
		
		return tab[k+7]*(2.0f-m) + tab[k+8]*(m-1.0f);
	}
};
//@LISTING{end:csection_t}

//@LISTING{start:csection_set_t}
struct csection_set_t {
	// number of channels
	u16  ncsect;
	// number of tabular points per cross-section
	u16  tsize;
	// max energy (associated with 'tsize')
	f32  max_energy;
	// m/M, ionization parameters, etc., (what else?).
	f32 *cffts;
	// look-up table:
	// rmax[k]    = tabs[k=0..ncsect]
	// enth[k]    = tabs[ncsect + tsize*(k=0..ncsect)]
	// rate[k][j] = tabs[nscect + tsize*(k=0..ncsect)+(j=1..tsize)]
	f32 *tabs;
	// microcode to check for events
	mprog_t *progs;
	
	/****************************************************************************/
	inline csection_t operator [] (u16 k) const {
		return csection_t(tabs + ncsect + tsize*k);
	};
	
	inline u16 search (f32 r0, u16 j, u16 n) const {
		u16  lf{0}, rt{n}, mid;
		while (lf < rt) {
			mid = (lf+rt)/2;
			if (r0 < tabs[j+mid]) {
				rt = mid;
			} else {
				lf = mid+1;
			}
		}
		return lf;
	};
	
};
//@LISTING{end:csection_set_t}
