/* * * * *
 *  AzReg_TsrSib.hpp 
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

#ifndef _AZ_REG_TSRSIB_HPP_
#define _AZ_REG_TSRSIB_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzRegDepth.hpp"
#include "AzReg_TreeReg.hpp"

//! 
class AzReg_TsrSib : /* implements */ public virtual AzReg_TreeReg {
protected:
  const AzTrTree_ReadOnly *tree; 

  int focus_nx; 
  const AzRegDepth *reg_depth; 
  bool forNewLeaf; 

  AzDataArray<AzSvect> av_dv; 
  AzDvect v_v; 

  double vdv_sum, dv2_sum, dr, ddr; 
  double newleaf_dep_factor; 

  virtual void reset_values() {
    vdv_sum = dv2_sum = dr = ddr = 0; 
    newleaf_dep_factor = 1; 
  }

public:
  AzReg_TsrSib()
    : tree(NULL), forNewLeaf(false), focus_nx(-1), 
      reg_depth(NULL), newleaf_dep_factor(1), 
      vdv_sum(0), dv2_sum(0), dr(0), ddr(0) {}

  void copyParam_from(const AzReg_TsrSib *inp) {}

  /*---------------------------------------------------------*/
  virtual void reset(const AzTrTree_ReadOnly *inp_tree, 
                     const AzRegDepth *inp_reg_depth); 
  /*---------------------------------------------------------*/

  virtual void penalty_deriv(int nx, double *dr, 
                             double *ddr); 

  virtual void changeWeight(int nx, double w_diff); 

  inline void clearFocusNode() {
    focus_nx = -1; 
  }

  /*---  for node split  ---*/
  /*---------------------------------------------------------*/
  /*---  called by AzRgf_FindSplit_TR::begin  ---*/
  //! set current penalty 
  virtual void reset_forNewLeaf(const AzTrTree_ReadOnly *t, 
                                const AzRegDepth *rdep); 
  /*---  called by AzRgf_FindSplit_TR::findSplit  ---*/
  virtual void reset_forNewLeaf(int f_nx, 
                      const AzTrTree_ReadOnly *t, 
                      const AzRegDepth *rdep); 
  /*---------------------------------------------------------*/

  virtual double penalty_diff(const double leaf_w_delta[2]) const; 
  virtual void penalty_deriv(double *dr, 
                             double *ddr) const; 

  /*---  for maintenance  ---*/
  virtual void show(const AzOut &out, 
                    const char *header) const {

  }

  /*---------------------------------------------------------*/
  virtual void resetParam(AzParam &param) {}
  virtual void printParam(const AzOut &out) const {}
  virtual void printHelp(AzHelp &h) const {}

  virtual inline const char *signature() const {
    return "-___-_RGF_TsrSib_"; 
  }
  virtual inline const char *description() const {
    return "RGF w/min-penalty regularization w/sum-to-zero sibling constraints"; 
  }
  /*---------------------------------------------------------*/

protected:
  void checkLeaf(const char *msg) const {
    if (!forNewLeaf || focus_nx < 0) {
      throw new AzException("AzReg_TsrSib::checkLeaf", msg); 
    }
  }
  virtual void deriv_v(const AzTrTree_ReadOnly *tree, 
                           int leaf_nx, 
                           bool forNewLeaf, 
                           /* output */
                           AzSvect *v_dv, 
                           /* inout */
                           AzDvect *v_v) const; 

  inline int get_newleaf_depth() const {
    return tree->node(focus_nx)->depth + 1; 
  }
  virtual void update(); 
}; 
#endif 

