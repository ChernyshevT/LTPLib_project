#ifdef BACKEND_DEBUG
#include <cstdio>
#endif
/******************************************************************************/
#pragma once
#include "typedefs.hxx"
#include <tgmath.h>
#include <math.h>

/******************************************************************************/

// [check & implement (if it is needed)]
// S. Mattei, K. Nishida, M. Onai, J. Lettry, M.Q. Tran, A. Hatayama
// doi:10.1016/j.jcp.2017.09.015
// A fully-implicit Particle-In-Cell Monte Carlo Collision code for the simulation of inductively coupled plasmas



inline void cross_product
(f32 r[], const f32 lhs[], const f32 rhs[]) {
	r[0] = lhs[1]*rhs[2] - lhs[2]*rhs[1];
	r[1] = lhs[2]*rhs[0] - lhs[0]*rhs[2];
	r[2] = lhs[0]*rhs[1] - lhs[1]*rhs[0];
}

//~ Particle simulation of plasmas: review and advances
//~ https://ieeexplore.ieee.org/abstract/document/6782717
//~ https://www.sciencedirect.com/science/article/pii/S2590055220300317:
//~ Two-dimensional leapfrog scheme for trajectories of relativistic charged particles in static axisymmetric electric and magnetic field
//~ Full Particle-In-Cell Simulation Methodology for Axisymmetric Hall Effect Thrusters
//~ Comparing Two-Dimensional, Axisymmetric, Hybrid-Direct Kinetic and Hybrid-Particle-in-Cell Simulations of the Discharge Plasma in a Hall Thruster

//~ next
//~ https://scholar.google.com/scholar?start=20&q=plasma+leapfrog+axisymmetric&hl=en&as_sdt=0,5


//@LISTING{start:update_v_boris}
// Updates velocity using the Boris method:
// Birdsall, Plasma Physics via Computer Simulation, p.62
// https://www.particleincell.com/wp-content/uploads/2011/07/ParticleIntegrator.java

// v: ($\vb{v}(t-{\delta t}/2) \rightarrow \vb{v}(t+{\delta t}/2)$) 
// fpt: $\frac{e\delta t}{2m}\qty[E^*_x, E^*_y, E^*_z, B^*_x/c, B^*_y/c, B^*_z/c]$
inline void update_v_boris (f32 v[], const f32 fpt[]) {
	const f32 *e{fpt}, *b{fpt+3};

	// s vector
	f32 s[3], bb{b[0]*b[0] + b[1]*b[1] + b[2]*b[2]};
	for (size_t j=0; j<3; j++) {
		s[j] = 2.0f*b[j]/(1.0f+bb);
	}

	// v minus
	f32 v_minus[3];
	for (size_t j=0; j<3; j++) {
		v_minus[j] = v[j] + e[j];
	}

	// v prime
	f32 v_prime[3], v_minus_cross_t[3];
	cross_product(v_minus_cross_t, v_minus, b);
	for (size_t j=0; j<3; j++) {
		v_prime[j] = v_minus[j] + v_minus_cross_t[j];
	}

	// v plus
	f32 v_plus[3], v_prime_cross_s[3];
	cross_product(v_prime_cross_s, v_prime, s);
	for (size_t j=0; j<3; j++) {
		v_plus[j] = v_minus[j] + v_prime_cross_s[j];
	}
	
	// v(t+dt/2)
	for (size_t j=0; j<3; j++) {
		v[j] = v_plus[j] + e[j];
	}
}
//@LISTING{end:update_v_boris}

//@LISTING{start:update_v_implicit}
// Updates velocity using the Borodachev method:
// Borodachev L.V. \& Kolomiets D.O.,
// Mathematical Models and Computer Simulations, volume 3, pages 357--364 (2011)
// doi: 10.1134/S2070048211030045

// current velocity -> intermediate velocity
// v: $\vb{v}\qty(t) \rightarrow \vb{u}\qty(t+{\delta t}/2)$
// fpt: $\frac{{e\delta t}}{2m}\qty[\vb{E}_*, \vb{B}_*/c]\qty(t+{\delta t})$
inline void update_v_predict
(const f32 v[], f32 u[], const f32 fpt[]) {
	const f32 *e{fpt}, *b{fpt+3};
	
	u[0] = v[0] + e[0] + b[2]*v[1] - b[1]*v[2];
	u[1] = v[1] + e[1] + b[0]*v[2] - b[2]*v[0];
	u[2] = v[2] + e[2] + b[1]*v[0] - b[0]*v[1];
}

// intermediate velocity -> next velocity
// v: $\vb{u}\qty(t+{\delta t}/2) \rightarrow \vb{v}\qty(t+{\delta t})$
// fpt: $\frac{{e\delta t}}{2m}\qty[\vb{E}_*, \vb{B}_*/c]\qty(t+{\delta t})$
inline void update_v_correct (f32 v[], const f32 u[], const f32 fpt[]) {
	const f32 *e{fpt}, *b{fpt+3};

	const f32
		axx{u[0]+e[0]}, ayy{u[1]+e[1]}, azz{u[2]+e[2]},
		bxx{b[0]*b[0]}, byy{b[1]*b[1]}, bzz{b[2]*b[2]},
		bxy{b[0]*b[1]}, byz{b[1]*b[2]}, bzx{b[2]*b[0]};
	const f32
		cff{1.0f/(1.0f+bxx+byy+bzz)};

	v[0] = cff*(axx*(1.0f+bxx) + ayy*(bxy+b[2]) + azz*(bzx-b[1]));
	v[1] = cff*(axx*(bxy-b[2]) + ayy*(1.0f+byy) + azz*(byz+b[0]));
	v[2] = cff*(axx*(bzx+b[1]) + ayy*(byz-b[0]) + azz*(1.0f+bzz));
}
//@LISTING{end:update_v_implicit}

//@LISTING{start:push_part}
// moves particle using following methods
// pt:  $\qty[\vb{r}_{x(yz)}, \vb{v}_{xyz}]$
// fpt: $\frac{e{\delta t}}{2m}\qty[\vb{E}_*, \vb{B}_*/c]$
// sdt: $\frac{\delta t}{2}\qty[1/{x(yz)}]$
template<u8 nd, u8 pushm, u8 clcrd=0>
inline void push_pt
(f32 pt[], const f32 fpt[], const f32 dt) noexcept {

	#if defined(BACKEND_DEBUG) || defined(FUNC_DEBUG)
	bool _debug_check{false};
	f32 p0[nd+3];
		for (u8 i{0u}; i<nd+3; ++i) {
			p0[i] = pt[i];
			_debug_check or_eq (p0[i] != p0[i]);
		}
	#endif

	// explicit (boris method)
	if constexpr (pushm == PUSH_MODE::LEAPF) {
		update_v_boris(pt+nd, fpt);
		
		if constexpr (clcrd and nd == 2) {
			f32 vx,vy,vz,x,y,z,r,a,sina,cosa;
			
			x  = pt[0];
			y  = pt[1];
			vx = pt[2];
			vy = pt[3];
			vz = pt[4];
			x  = vx*dt + x;
			y  = vy*dt + y;
			z  = vz*dt;
			r  = sqrtf(y*y + z*z);
			/*
			if (r >= FLT_EPSILON) {
				cosa = y/r;  sina = z/r;
			} else {
				cosa = 1.0f; sina = 0.0f;
			}
			*/
			a  = atan2(z, y);
			sina = sin(a);
			cosa = cos(a);
			pt[0] = x;
			pt[1] = r;
			pt[2] = vx;
			pt[3] = vy*cosa + vz*sina;
			pt[4] = vz*cosa - vy*sina; 
		} else for (size_t i{0}; i<nd; ++i) {
			pt[i] = pt[i] + pt[nd+i]*dt;
		}
	}
	
	// implicit (first run)
	if constexpr (pushm == PUSH_MODE::IMPL0) {
		f32 *sh{pt+nd+3};
		update_v_predict(pt+nd, sh+nd, fpt);
		for (size_t i{0}; i<nd; ++i) {
			sh[i] = pt[i] + pt[nd+i]*dt;
		}
		//~ update_v_correct(pt+nd, sh+nd, fpt);
		//~ for (size_t i{0}; i<nd; ++i) {
			//~ pt[i] = sh[i] + pt[nd+i]*dt;
		//~ }
		for (u8 i{0u}; i<nd+3; ++i) {
			pt[i] = sh[i];
		}
	}
	
	// implicit (second run)
	if constexpr (pushm == PUSH_MODE::IMPLR) {
		f32 *sh{pt+nd+3};
		update_v_correct(pt+nd, sh+nd, fpt);
		for (size_t i{0}; i<nd; ++i) {
			pt[i] = sh[i] + pt[nd+i]*dt;
		}
	}
	
	#if defined(BACKEND_DEBUG) || defined(FUNC_DEBUG)
	for (u8 i{0u}; i<nd+3; ++i) {
		_debug_check or_eq (pt[i] != pt[i]);
	}
	if (_debug_check) {
		#pragma omp critical
		{
			setbuf(stdout, nullptr);
			printf("bad sample in push_pt:\n");
			for (u8 i{0u}; i<nd+3; ++i) {
				if (i<nd) {
					printf("r%c %10.3f -> %10.3f\n", char('x'+i),    p0[i], pt[i]);
				} else {
					printf("v%c %+10.3e -> %+10.3e\n", char('x'+i-nd), p0[i], pt[i]);
				}
			}
			printf("fpt ="); for (u8 i{0u}; i<6; ++i) {
				printf(" %+10.3e", fpt[i]);
			}
			printf("\n");
		}
	}
	#endif
}
//@LISTING{end:push_part}
