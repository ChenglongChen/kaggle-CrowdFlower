/* * * * *
 *  AzReg_Tsrbase.hpp 
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

#ifndef _AZ_REG_TSRBASE_HPP_
#define _AZ_REG_TSRBASE_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"
#include "AzTrTree_ReadOnly.hpp"
#include "AzRegDepth.hpp"
#include "AzReg_TreeReg.hpp"
#include "AzParam.hpp"

//! 
class AzReg_Tsrbase : /* implements */ public virtual AzReg_TreeReg
{
protected:
  const AzTrTree_ReadOnly *tree; 
  AzDataArray<AzDvect> av_dbar; 
  AzDvect v_bar; 

  /*---  used only by OptOnTree  ---*/
  AzDataArray<AzDvect> av_dv; 
  AzDvect v_v; 
  AzDvect v_dv2_sum; 
  /*--------------------------------*/

  int focus_nx; 
  const AzRegDepth *reg_depth; 
  bool forNewLeaf; 

  double penalty_offset; 
  double vdv_sum, dv2_sum, dr, ddr; 
  double newleaf_dep_factor; 
  double newleaf_v, focus_dbar; /* to speed up penalty_diff */

public:
  AzReg_Tsrbase()
    : tree(NULL), forNewLeaf(false), focus_nx(-1), reg_depth(NULL), penalty_offset(0), 
      vdv_sum(0), dv2_sum(0), dr(0), ddr(0), newleaf_dep_factor(1), 
      newleaf_v(0), focus_dbar(0) {}

  virtual void copyParam_from(const AzReg_Tsrbase *inp) {}

  virtual void reset_values() {
    penalty_offset = 0; 
    vdv_sum = dv2_sum = dr = ddr = 0; 
    newleaf_dep_factor = 1; 
    newleaf_v = focus_dbar = 0; 
  }

  /*****  for optimization  *****/
  /*---  called by AzOptOnTree_TreeReg  ---*/
  virtual void reset(const AzTrTree_ReadOnly *inp_tree, 
                const AzRegDepth *inp_reg_depth) {
    _reset(inp_tree, inp_reg_depth); 
    reset_values();  
  }

  inline virtual void clearFocusNode() {
    focus_nx = -1; 
  }

  virtual void penalty_deriv(int nx, double *dr, 
                             double *ddr); 

  virtual void changeWeight(int nx, double w_diff); 

  /******  for node split  *****/
  /*---------------------------------------------------------*/
  /*---  called by AzRgf_FindSplit_...::begin for each tree  ---*/
  //! set current penalty 
  virtual void reset_forNewLeaf(const AzTrTree_ReadOnly *t, 
                                const AzRegDepth *rdep) {
    _reset_forNewLeaf(t, rdep); 
    reset_values(); 
  }
  /*---  called by AzRgf_FindSplit_...::findSplit for each node  ---*/
  virtual void reset_forNewLeaf(int f_nx, 
                      const AzTrTree_ReadOnly *t, 
                      const AzRegDepth *rdep) {
    penalty_offset = _reset_forNewLeaf(f_nx, t, rdep); 
    update(); 
  }
  /*---------------------------------------------------------*/

  virtual double penalty_diff(const double leaf_w_delta[2]) const; 
  virtual void penalty_deriv(double *dr, 
                             double *ddr) const; 

  /*---  for maintenance  ---*/
  virtual void show(const AzOut &out, 
                    const char *header) const {}

  /*---------------------------------------------------------*/
  virtual void resetParam(AzParam &param) {}
  virtual void printParam(const AzOut &out) const {}
  virtual void printHelp(AzHelp &h) const {}
  /*---------------------------------------------------------*/

  double get_v(int nx) const; 
  double get_dv(int nx) const; 
  double penalty() const; 

protected:
  /*---------------------------------------------------------*/
  virtual void _reset(const AzTrTree_ReadOnly *inp_tree, 
                      const AzRegDepth *inp_reg_depth) = 0; 
  /*---------------------------------------------------------*/

  /*---  for node split  ---*/
  /*---------------------------------------------------------*/
  /*---  called by AzRgf_FindSplit_...::begin for each tree  ---*/
  //! set current penalty 
  virtual void _reset_forNewLeaf(const AzTrTree_ReadOnly *t, 
                                 const AzRegDepth *rdep) = 0; 
  /*---  called by AzRgf_FindSplit_...::findSplit for each node  ---*/
  virtual double _reset_forNewLeaf(int f_nx, 
                      const AzTrTree_ReadOnly *t, 
                      const AzRegDepth *rdep) = 0; 
  /*---------------------------------------------------------*/

  inline void checkLeaf(const char *msg) const {
    if (!forNewLeaf || focus_nx < 0) {
      throw new AzException("AzReg_Tsrbase:checkLeaf", msg); 
    }
  }
  inline void checkFocus(const char *msg) const {
    if (focus_nx < 0) {
      throw new AzException("AzReg_Tsrbase::checkFocus", msg); 
    }
  }

  inline int get_newleaf_depth() const {
    return tree->node(focus_nx)->depth + 1; 
  }
  inline double get_newleaf_v() const {
    return tree->node(focus_nx)->weight - v_bar.get(focus_nx);
  }
  inline double get_newleaf_dv() const {
    return 1 - av_dbar.point(focus_nx)->get(focus_nx);
  }
  inline double get_newleaf_sib_dv() const {
    return 0 - av_dbar.point(focus_nx)->get(focus_nx);
  }
  double get_dv2_sum() const; 

  void update(); 
}; 
#endif 
