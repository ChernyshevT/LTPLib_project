#pragma once
#include "typedefs.hxx"
#include "api_csection_set.hxx"
#include "common/collisions.hxx"
#include "common/rng.hxx"

collision_t inline mcrun
(rng_t& rng, const csection_set_t& cset, u8 tag, f32 v1[], f32 bg[], f32 dt) {
	// spawn object to return
	collision_t cl(rng);
	// vars...
	f32 R0{rng.uniform(0.0f, 1.0f)}, R1, N0dt{0.0f};
	// current channel id, shift to jump, number of chnnels in the current set
	u16 j{0}, k, n;
	
	auto cmd {cset.progs+tag};
	next: switch (cmd->opc) {
		
		case opcode::JMP:
			cmd += cmd->arg;
		goto next;
		
		case opcode::SETCSID:
			j = cmd->arg;
			cmd += 1;
		goto next;
		
		case opcode::MASSRATE:
			cl.mrate = cset.cffts[cmd->arg];
			cmd += 1;
		goto next;
		
		case opcode::SELECTBG:
			cl.bgid = cmd->arg;
			N0dt = bg[cmd->arg]*dt;
			cmd += 1;
		goto next;
		
		case opcode::SEARCH:
			n = cmd->arg;
			k = cset.search(&R1, R0/N0dt, j, n); /* R1 will be modified */
			if (k < n) {
				auto entry = cset[j+k];
				f32 enel = cl.do_energy(v1, cset.cffts[tag]);
				f32 enth = entry.enth;
				if (enel > enth and R0 < entry[enel]*N0dt) {
					if (enel >= cset.max_energy) {
						cl.type = cltype::ERROR_ENLIMIT;
						goto end;
					}
					cl.chnl  = j+k+1;
					cl.enth  = enth;
					cl.ensys = enel-enth;
				} else {
					k = n;
				}
			}
			j += k;
			R0  -= R1*N0dt;
			cmd += 1+k;
		goto next;
		
		// conservative
		case opcode::ELASTIC:
		case opcode::EXCITATION:
		case opcode::VIBRATIONAL:
		case opcode::ROTATIONAL:
		case opcode::DISSOCIATION:
			cl.type = cltype::CONSERVATIVE;
			cmd += cmd->arg;
		goto next;
		
		// non-conservative
		case opcode::IONIZATION:
			cl.type = cltype::IONIZATIONRUN;
			cl.nsee = 1;
			cmd += cmd->arg;
			goto next;
		
		case opcode::ATTACHMENT:
			cl.type = cltype::ATTACHMENT;
			cmd += cmd->arg;
		goto next;
		
		// extra parameters
		case opcode::SETDCSFACTOR:
			cl.param = cset[cmd->arg][cl.enel];
			cmd += 1;
		goto next;

		case opcode::SETOPBFACTOR:
			cl.param = cset.cffts[cmd->arg];
			cmd += 1;
		goto next;
		
		// final check (opcode::END) 
		default: if (R0 < -0.25f) { // TODO: make this variable
			cl.type = cltype::ERROR_PROBMAX;
			cl.chnl = 0;
		}
		
	}
	end: return cl;
}
