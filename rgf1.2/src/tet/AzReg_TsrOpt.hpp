/* * * * *
 *  AzReg_TsrOpt.hpp 
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

#ifndef _AZ_REG_TSROPT_HPP_
#define _AZ_REG_TSROPT_HPP_

#include "AzRgf_kw.hpp"
#include "AzReg_Tsrbase.hpp"

//! 
class AzReg_TsrOpt : /* extends */ public virtual AzReg_Tsrbase
{
protected:
  int reg_ite_num; 
  double curr_penalty; 

  AzDmat *m_coeff; /* shared with the regularizers for other trees */
                   /* owned by AzReg_TreeRegShared */

  //! tree structure 
  AzIIarr iia_le_gt; 

public:
  AzReg_TsrOpt() : reg_ite_num(reg_ite_num_dflt), curr_penalty(0), m_coeff(NULL) {}

  virtual void copyParam_from(const AzReg_TsrOpt *inp) {
    AzReg_Tsrbase::copyParam_from(inp); 
    reg_ite_num = inp->reg_ite_num; 
  }

  void set_shared(AzReg_TreeRegShared *ptr) {
    m_coeff = ptr->share(); 
  }
  virtual void check_reg_depth(const AzRegDepth *rd) const {
    if (rd == NULL) return; 
    rd->check_if_nonincreasing("min-penalty regularizers"); 
  }

  /*---------------------------------------------------------*/
  virtual void _reset(const AzTrTree_ReadOnly *inp_tree, 
                      const AzRegDepth *inp_reg_depth); 
  /*---------------------------------------------------------*/

  /*---  for node split  ---*/
  /*---------------------------------------------------------*/
  /*---  called by AzRgf_FindSplit_TR::begin for each tree  ---*/
  //! set current penalty 
  virtual void _reset_forNewLeaf(const AzTrTree_ReadOnly *t, 
                                const AzRegDepth *rdep); 
  /*---  called by AzRgf_FindSplit_TR::findSplit for each node  ---*/
  virtual double _reset_forNewLeaf(int f_nx, 
                      const AzTrTree_ReadOnly *t, 
                      const AzRegDepth *rdep); 
  /*---------------------------------------------------------*/

  /*---  for maintenance  ---*/
  virtual void show(const AzOut &out, 
                    const char *header) const {
    show(out, header, reg_depth, focus_nx, &av_dbar, tree, &v_bar); 
  }

  /*---------------------------------------------------------*/
  virtual void resetParam(AzParam &param); 
  virtual void printParam(const AzOut &out) const; 
  virtual void printHelp(AzHelp &h) const; 

  virtual inline const char *signature() const {
    return "-___-_RGF_TsrOpt_"; 
  }
  virtual inline const char *description() const {
    return "RGF w/min-penalty regularization"; 
  }
  /*---------------------------------------------------------*/

  /*---  static tools  ---*/
  static void show(const AzOut &out, 
                    const char *header, 
                   const AzRegDepth *reg_depth, 
                    int focus_nx, 
                   const AzDataArray<AzDvect> *av_dbar, 
                    const AzTrTree_ReadOnly *tree, 
                    const AzDvect *v_bar); 
  static void _propagate(int ite_num, 
                         const AzTrTree_ReadOnly *tree, 
                         int split_nx, 
                         const double new_leaf_w[2], 
                         const AzIntArr *ia_nonleaf, 
                         const AzRegDepth *reg_depth, 
                         AzDmat *m_coeff, /* inout */
                         AzDvect *v); /* output */

  static void _show(int focus_nx, 
                    const AzDataArray<AzDvect> *av_dbar, 
                    const AzTrTree_ReadOnly *tree, 
                    const AzDvect *v_bar, 
                    int nx, 
                    const AzDmat *m_coeff, 
                    const AzOut &out);  

  static void setCoeff(const AzRegDepth *reg_depth, int depth, double coeff[4]); 
  static void setCoeff(const AzRegDepth *reg_depth, 
                       const AzTrTree_ReadOnly *tree, 
                       AzDmat *m_coeff); 

protected:
  void resetTreeStructure(); 
  bool isSameTreeStructure() const; 
  void storeTreeStructure(); 

  void update_v(); 
  void update_dv(); 
  void reset_v_dv() {
    av_dv.reset(); 
    v_v.reset(); 
    v_dv2_sum.reset(); 
  }

  /*---  compute "bar" (auxiliary variables) iteratively  ---*/
  virtual void reset_bar(int split_nx, 
                  const AzIntArr *ia_leaf, 
                  const AzIntArr *ia_nonleaf); 
  /*---  compute "bar"'s derivatives iteratively  ---*/
  virtual void deriv(int base_nx, /* derivative w.r.t. this node's weight */
                    const AzIntArr *ia_nonleaf, 
                    AzDvect *v_dbar);

}; 
#endif 

