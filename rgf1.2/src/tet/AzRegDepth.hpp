/* * * * *
 *  AzRegDepth.hpp 
 *  Copyright (C) 2011, 2012 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZ_REG_DEPTH_HPP_
#define _AZ_REG_DEPTH_HPP_

#include "AzRgf_kw.hpp"
#include "AzUtil.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"
#include "AzRegDepth.hpp"

#define depth_base_dflt 1
/* #define depth_base_min_penalty_dflt 2 */

//! Regularizer using node depth. 
class AzRegDepth {
protected:
  double depth_base; 
  AzDvect v_dep2pow;  /* to avoid repetitive calls of pow() */
  const double *dep2pow; 

public:
  AzRegDepth() : depth_base(depth_base_dflt), dep2pow(NULL) {}

  virtual void set_default_for_min_penalty() {
    /* depth_base = depth_base_min_penalty_dflt; */
  }

  virtual inline 
  void check_if_nonincreasing(const char *who) const {
    if (depth_base < 1) {
      AzBytArr s(kw_depth_base); s.c(" must be no smaller than 1 for "); 
      s.c(who); s.c("."); 
      throw new AzException(AzInputNotValid, "AzRegDepth::check_if_nonincreasing", 
                            s.c_str()); 
    }
  }

  virtual inline 
  double apply(double val, int dep) const {
    if (depth_base == 1) return val; 
    if (dep >= 0 && dep < v_dep2pow.rowNum()) {
      return val * dep2pow[dep]; 
    }
    else {
      return val * pow(depth_base, (double)dep); 
    }
  }

  virtual void reset(AzParam &param, 
             const AzOut &out) {
    resetParam(param); 
    if (depth_base <= 0) {
      throw new AzException(AzInputNotValid, "AzRegDepth::reset", 
            kw_depth_base, "must be no smaller than 1."); 
    }
    if (depth_base < 1) {
      AzBytArr s("!Warning! "); s.c(kw_depth_base); s.c(" should be no smaller than 1."); 
      AzPrint::writeln(out, s); 
    }

    printParam(out); 
  }
  virtual void printHelp(AzHelp &h) const {
    h.begin(Azforest_config, "AzRegDepth", "Regularization on node depth"); 
    h.item(kw_depth_base, help_depth_base, depth_base); 
    h.end(); 
  }
  virtual void printParam(const AzOut &out) const {
    if (out.isNull()) return; 
    if (depth_base != 1) {
      AzPrint o(out); 
      o.ppBegin("AzRegDepth", "Reg. on depth", ", "); 
      o.printV(kw_depth_base, depth_base);
      o.ppEnd(); 
    }
  }

protected:
  virtual void resetParam(AzParam &p) {
    bool doCheck = false; 
    p.vFloat(kw_depth_base, &depth_base); 

    /*---  ---*/
    v_dep2pow.reform(50);
    int dep; 
    for (dep = 0; dep < v_dep2pow.rowNum(); ++dep) {
      v_dep2pow.set(dep, pow(depth_base, (double)dep)); 
    }
    dep2pow = v_dep2pow.point(); 
  }
}; 
#endif 
