#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
namespace py = pybind11;
using namespace pybind11::literals;

#include <vector>
#include <iostream>
#include <complex.h>
#include "typedefs.hxx"
#include "io_strings.hxx"

//https://math.stackexchange.com/questions/2706701/neumann-boundary-conditions-in-finite-difference


/*
CENTER:
hx = 1/dx/dx
u0*(2*hx+2*hy+2*hz) = hx*(uxl+uxr) + hy*(uyl+uyr) + hz*(uzl+uzr) - q

NEWMANN:

u0*(2*hx+2*hy+2*hz) = 2*hx*(uxr)-2*Ex0/dx  + hy*(uyl+uyr) + hz*(uzl+uzr) - q

*/

/***************** node zz xx yy **********************************************/
#define SETVALUE   0b01'00'00'00 // boundary condition (Dirichlet)
#define SETAXIS    0b10'00'00'00 // mark finite differences
#define LFDIFF     0b00'00'00'01 // left finite difference
#define RTDIFF     0b00'00'00'10 // right finite difference
#define CNDIFF     0b00'00'00'11 // central finite difference

#define CHECK_UNIT(arg)     (0b11'00'00'00 &  (arg))
#define CHECK_AXIS(arg, ax) (0b00'00'00'11 & ((arg) >> ((ax) * 2)))

enum uTYPE : u8 { // unit type
	NIL = 0,
	VAL = SETVALUE,
	
	XLF = SETAXIS | (LFDIFF<<0),
	XRT = SETAXIS | (RTDIFF<<0),
	XCN = SETAXIS | (CNDIFF<<0),

	YLF = SETAXIS | (LFDIFF<<2),
	YRT = SETAXIS | (RTDIFF<<2),
	YCN = SETAXIS | (CNDIFF<<2),

	ZLF = SETAXIS | (LFDIFF<<4),
	ZRT = SETAXIS | (RTDIFF<<4),
	ZCN = SETAXIS | (CNDIFF<<4),
};
/******************************************************************************/

template<u8 nd, typename tp=f32 /* real or complex [TBD] */>
struct SOR_solver_t {
	u64  offst[nd+1]; // nx*ny*nz*1
	u32  shape[nd];
	f32  dstep[nd];   // 1/dx/dx
	u8  *umap;        // cell-unit type;
	f32 *cdata;       // 
	f32 *pvdata[2];   // voltage (red & black units)
	
	/* pos: ix,iy,iz; nseq: red (0) -> black (1) */
	inline f32 get_vnew (u32 pos[nd], u8 nseq) {
		
		// mid-point, left, right indexes
		u64 k0{0}, kL, kR;
		for (u8 i{0u}; i<nd; ++i) {
			k0 += offst[i+1] * pos[i];
		}
		
		f32 *vdata{pvdata[nseq]};
		u8   ucode{umap[k0]};
		f32  vnew{0.0f}, cfft{0.0f};

		/* stencil:
		 *           [a][a]   
		 *            |/      
		 *  y z  [b]-[0]-[a]
		 *  |/       /|
		 *  0-x   [b][b]
		 **/

		/* check the unit to perfornm the action **********************************/
		switch CHECK_UNIT(ucode) {
			
			default:
				vnew = NAN;
				break;
			
			case SETVALUE: // [0]
				vnew = vdata[k0];
				break;
			
			// loop over each axis & update vnew
			case SETAXIS:
				for (u8 j{0u}; j<nd; ++j) switch CHECK_AXIS(ucode, j) {
					
					default:
						continue;
					
					case LFDIFF: // ([b]-[0])/(dx*dx)
						kL = 0;
						for (u8 i{0u}; i<nd; ++i) {
							kR += offst[i+1] * (pos[i]-(i==j));
						}
						vnew += 2*dstep[j]*vdata[kL];
						cfft += 2*dstep[j];
						continue;
					
					case RTDIFF: // ([a]-[0])/(dx*dx)
						kR = 0;
						for (u8 i{0u}; i<nd; ++i) {
							kR += offst[i+1] * (pos[i]+(i==j));
						}
						vnew += 2*dstep[j]*vdata[kR];
						cfft += 2*dstep[j];
						continue;
					
					case CNDIFF: // ([a]-[c])/(2*dx*dx)
						kL = 0; kR = 0;
						for (u8 i{0u}; i<nd; ++i) {
							kL += offst[i+1] * (pos[i]-(i==j));
							kR += offst[i+1] * (pos[i]+(i==j));
						}
						vnew += (vdata[kL]+vdata[kR])*dstep[j];
						cfft += 2*dstep[j];
						continue;
				}
				vnew = (vnew-cdata[k0])/cfft;
				break;
		}
		return vnew;
	}
	
/*******************************************************************************
 * An implementation for SOR+GS method, see [Mittal(2014) S Mittal.
 * A study of successive over-relaxation method parallelisation over modern
 * HPC languages. IJHPCN. 7(4):292, 2014. doi: 10.1504/ijhpcn.2014.062731].
 ******************************************************************************/ 
 
	inline f32 iter (f32 w) {
	/*****************************************************************************
	 * red:   pvdata[0] -> pvdata[1]
	 * black: pvdata[1] -> pvdata[1]
	 * SOR:   pvdata[1] -> pvdata[0]
	 ****************************************************************************/
		
		//~ fmt::print("{}\n", 40*"*"s);
		//~ fmt::print("cache old values:\n");
		/* cache old values */
		#pragma omp parallel for
		for (u64 uid=0; uid<offst[0]; ++uid) {
			pvdata[1][uid] = pvdata[0][uid];
			
			//~ fmt::print("{:06d}: {:+e}\n", uid, pvdata[1][uid]);
		}

		/* loop over red/black-units */
		for (u8 nseq : {0,1}) {
			
			//~ fmt::print("{}\n", 40*"*"s);
			//~ fmt::print("{} sequence:\n", nseq==0? "red":"black");
			/* loop over nodes */
			//~ #pragma omp parallel for reduction(max:verr) private(vold, vnew)
			#pragma omp parallel for
			for (u64 uid=0; uid<offst[0]; ++uid) {
				u32 pos[nd];
				u32 sum{0};
				u64 remain{uid};
				for (u8 i{0u}; i<nd; ++i) {
					pos[i] = remain / offst[i+1];
					remain = remain % offst[i+1];
					sum += pos[i];
				}
				if ( (u8)(sum%2) == nseq) {
					
					pvdata[1][uid] = get_vnew(pos, nseq);
					//~ pvdata[1][uid] = nseq==0? (float)uid: -1.0f*(float)uid;
					//~ fmt::print("{:c} (sum={:06d}) #{:06d} x{:02d} y{:02d} -> {:+e}\n"
					//~ , nseq==0?'R':'B', sum, uid, pos[0], pos[1], pvdata[1][uid]);
				}
				
			}
		}
		
		//~ fmt::print("{}\n", 40*"*"s);
		//~ fmt::print("finalization:\n");
		/* perform SOR-step */
		f32 verr{0.0f};
		#pragma omp parallel for reduction(max:verr)
		for (u64 uid=0; uid<offst[0]; ++uid) {
			f32 vold{pvdata[0][uid]};
			f32 vnew{pvdata[1][uid]};
			
			
			vnew = w*vnew + (1.0f-w)*vold;
			verr = std::max(fabsf(vnew - vold), verr);
			
			pvdata[0][uid] = vnew;
			
			//~ fmt::print("{:06d}: {:+e}\n", uid, pvdata[0][uid]);
		}
		
		return verr;
	}
};

template<u8 nd> using SOR_iter_t = void (SOR_solver_t<nd>&);

//~ template<u8 nd>


typedef std::variant<
	SOR_solver_t<1>,
	SOR_solver_t<2>,
	SOR_solver_t<3>
> SOR_solver_v;

typedef py::array_t<u8,  py::array::c_style> utype;
typedef py::array_t<f32, py::array::c_style> vtype;

/******************************************************************************/
struct poisson_eq_holder {
	
	SOR_solver_v solver;
	
	std::function<f32(f32)> iter_fn;
	
	/* ctor */
	 poisson_eq_holder (std::vector<f32> &step_arg, utype &umap_arg, vtype &cdata_arg, vtype &vdata_arg) {
		fmt::print("umap.ndim =  {}\n", umap_arg.request().ndim);
		
		switch (umap_arg.request().ndim) {
			case 1:
				this->solver = SOR_solver_v{SOR_solver_t<1>{}};
				break;
			case 2:
				this->solver = SOR_solver_v{SOR_solver_t<2>{}};
				break;
			case 3:
				this->solver = SOR_solver_v{SOR_solver_t<3>{}};
				break;
			default:
				throw bad_arg("invalid shape ({})", umap_arg.request().ndim);
		}
		/* validate input */
		if (umap_arg.request().ndim != cdata_arg.request().ndim
		or  umap_arg.request().ndim != vdata_arg.request().ndim
		or  umap_arg.request().ndim != (py::ssize_t)(step_arg.size())) throw bad_arg \
		("incompatataple input: umap.ndim = {}, cdata.ndim = {}, vdata.ndim ={}"
		, umap_arg.request().ndim, cdata_arg.request().ndim, vdata_arg.request().ndim);
		
		std::visit([&] <u8 nd> (SOR_solver_t<nd> &solver) {
			fmt::print("register poisson_eq<{}>\n", nd);
			
			solver.offst[nd] = 1;
			for (u8 i{0u}; i<nd; ++i) {
				//~ fmt::print("{}\n", nd-i-1);
				solver.shape[nd-i-1] = umap_arg.request().shape[nd-i-1];
				solver.offst[nd-i-1] = solver.offst[nd-i]*solver.shape[nd-i-1];
				solver.dstep[nd-i-1] = 1.0f/step_arg[nd-i-1]/step_arg[nd-i-1];
			}
			
			solver.umap  = (u8* )(umap_arg.request().ptr);
			solver.cdata = (f32*)(cdata_arg.request().ptr);
			
			solver.pvdata[0] = (f32*)(vdata_arg.request().ptr);
			solver.pvdata[1] = new f32[solver.offst[0]];
		}, this->solver);
		
		this->iter_fn = \
		std::visit([&] <u8 nd> (SOR_solver_t<nd> &solver) -> decltype(this->iter_fn) {
			return [&] (f32 w) -> f32 {
				return solver.iter(w);
			};
		}, this->solver); 
		
	} /* end ctor */
	
};
/******************************************************************************/

//~ bool validate_umap (u8 umap[], u32 shape[], u8 nd, u8 md=0) {
	//~ if (md < nd) {
	//~ } else {
	//~ }
//~ }

/******************************************************************************/

void def_poisson_eq(py::module &m) {
	fmt::print("register poisson eq (experimental)\n");
	
	py::class_<poisson_eq_holder> cls (m, "poisson_eq"); cls
	
	.def(py::init<std::vector<f32> &, utype &, vtype &, vtype &>()
	, py::keep_alive<1, 3>(), py::keep_alive<1, 4>(), py::keep_alive<1, 5>())
	
	.def("iter", [] (poisson_eq_holder& self, f32 w) -> f32 {
		return self.iter_fn(w);
	})
	;
	
	py::enum_<uTYPE> (cls, "uTYPE", py::arithmetic())
	.value("NIL", uTYPE::NIL)
	.value("VAL", uTYPE::VAL)
	.value("XLF", uTYPE::XLF)
	.value("XRT", uTYPE::XRT)
	.value("XCN", uTYPE::XCN)
	.value("YLF", uTYPE::YLF)
	.value("YRT", uTYPE::YRT)
	.value("YCN", uTYPE::YCN)
	.value("ZLF", uTYPE::ZLF)
	.value("ZRT", uTYPE::ZRT)
	.value("ZCN", uTYPE::ZCN)
	.export_values()
	;
}

