#ifndef _COM_GRIDS_HEADER
#define _COM_GRIDS_HEADER
#include <cstdint>
#include <cstddef>

#include "typedefs.hxx"
#include "api_backend.hxx"

//@LISTING{start:forms}
// form-factors ($^d$-order clouds)
template <u8 nd, u8 order>
struct form_t {
	u32 idx [nd];
	f32 vals[nd*(order+1)];
};

template<u8 order>
u32 calc_form (f32 *frm, f32 x) {
	u32 k;
	
	if constexpr (order == FORM_ORDER::NEAR) { // nearest (1 point)
		k = u32(x);
		frm[0] = 1.0f;
	}
	
	if constexpr (order == FORM_ORDER::LINE) { // linear (2 points)
		f32 g;
		k = u32(x);
		
		g = x-f32(k);
		frm[0] = 1.0f-g;
		frm[1] = g;
	}
	
	if constexpr (order == FORM_ORDER::QUAD) { // square (3 points)
		f32 g, a, b;
		k = u32(x);

		g = x-f32(k)-0.5f;
		a = 0.50f-g;
		b = 0.50f+g;
		frm[0] = 0.50f*a*a;
		frm[1] = 0.75f-g*g;
		frm[2] = 0.50f*b*b;
	}
	
	if constexpr (order == FORM_ORDER::CUBE) { // cubic (4 points)
		f32 g, g2,g3, a, a2, a3;
		k = u32(x);

		g  = x-f32(k);
		g2 = g*g;
		g3 = g*g2;
		a  = 1.0f-g; 
		a2 = a*a;
		a3 = a*a2;
		frm[0] = a3/6.0f;
		frm[1] = 2.0f/3.0f - g2 + 0.5f*g3;
		frm[2] = 2.0f/3.0f - a2 + 0.5f*a3;
		frm[3] = g3/6.0f;
	}
	
	return k;
}
//@LISTING{end:forms}

//@LISTING{start:grid_t}
template<u8 nd>
struct grid_t {
public:
	// total number of sub-domains
	u32  size;
	// number of sub-domains along each axis
	u32  shape [nd];
	// grid step along each axis
	f32  step  [nd];
	// bounds for sub-domains along each axis
	u32 *axes  [nd];
	// edges for sub-domains along each axis
	f32 *edges [nd];
	// mask to identify particle absorbers
	u8  *mask;
	// search for node by position
	u32 *lctr;

	struct {
		// sub-domain position
		u32 map[nd];
		// links to the other sub-domains
		u32 lnk[(nd>1?(nd>2?27:9):3)-1];
		// mask's shift to identify particle absorbers (or 0 if there is none)
		u64 mshift;
	} *nodes;
	
	struct {
		//mark looped axes
		u8 loopax: 3;
		//mark 'x' as an axis of symmetry (2d grid only)
		u8 cylcrd: 1;
	} flags;

public:
	
	template<u8 i=nd-1>
	inline u32 find_node (f32 pos[], u32 sh=1, u32 k=0) const {
		u32 j{0}, j1{shape[i]+1}, md;
		while (j < j1) {
			md = (j + j1)/2;
			if (edges[i][md] <= pos[i]) {
				j  = md+1;
			} else {
				j1 = md;
			}
		}
		if constexpr (i > 0) {
			return find_node<i-1>(pos, sh*(shape[i]+2), k + j*sh);
		} else {
			return lctr[k+j*sh];
		}
	}

	struct node_t {
		u32
		 *map,
		 *lnk,
		  shape[nd],
		  shift[nd];
		u8
		   *mask;
		
		f32
			step[nd],
			edgel[nd],
			edger[nd],
			facel[nd],
			facer[nd],
			sdist[nd];

		template<u8 m=nd>
		inline u8 find_idir (f32 pt[], f32 sh[]) const {
			if constexpr (m==0) {
				return 0;
			} else {
				
				constexpr u8
					a = (m>1 ? (m>2 ? 18 : 6) : 2),
					b = (m>1 ? (m>2 ?  9 : 3) : 1),
					i = m-1;
				
				u8 idir = find_idir<m-1>(pt, sh);

				if        (pt[i] >= edger[i]) {
					if      (pt[i] >= facer[i]) {
						pt[i] -= sdist[i];
						if (sh) sh[i] -= sdist[i];
					} idir  += a;
				} else if (pt[i] <  edgel[i]) {
					if      (pt[i] <  facel[i]) {
						pt[i] += sdist[i];
						// check for overflow
						if    (pt[i] <  facer[i]) {
							if (sh) sh[i] += sdist[i];
							idir  += b;
						} else {
						// fix overflow
							pt[i]  = facel[i];
						}
					} else {
						idir  += b;
					}
				}
				return idir;
			}
		}
		
		inline u8 check_mask (u32 ptidx[]) {
			if (mask) {
				u32 k{0}, m{1}; for (u8 i{1}; i <= nd; ++i) {
					k += ptidx[nd-i] * m;
					m *= shape[nd-i];
				}
				return mask[k];
			} else {
				return 0;
			}
		}
		
		template<u8 order>
		inline u32 get_form (form_t<nd, order> *form, f32 pos[]) {
			u32 flag{0};
			for (u8 i{0}; i<nd; ++i) {
				form->idx[i] \
				= calc_form<order>(form->vals+i*(order+1), (pos[i]-edgel[i])/step[i]);
				
				flag |= (form->idx[i] >= shape[i])*ERR_CODE::PTOUTOFRANGE;
			}
			
			return flag;
		}
	};
	
	/****************************************************************************/
	node_t operator [] (size_t k) const {
		node_t node{nodes[k].map, nodes[k].lnk};
		for (auto i{0}; i<nd; ++i) {
			auto ax = axes[i];
			auto id = nodes[k].map[i];
			
			node.step[i] = step[i];
			node.shift[i] = ax[id];
			node.shape[i] = ax[id+1]-ax[id];
			node.facel[i] = step[i] * (f32)(ax[0]);
			node.edgel[i] = step[i] * (f32)(ax[id]);
			node.edger[i] = step[i] * (f32)(ax[id+1]);
			node.facer[i] = step[i] * (f32)(ax[shape[i]]);
			node.sdist[i] = node.facer[i]-node.facel[i];
		}
		node.mask = nodes[k].mshift ? mask+nodes[k].mshift-1 : nullptr;
		return node;
	}
};
//@LISTING{end:grid_t}
#endif
