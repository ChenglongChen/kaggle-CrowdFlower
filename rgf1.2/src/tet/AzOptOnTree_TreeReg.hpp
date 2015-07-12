/* * * * *
 *  AzOptOnTree_TreeReg.hpp 
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

#ifndef _AZ_OPT_ON_TREE_TREE_REG_HPP_
#define _AZ_OPT_ON_TREE_TREE_REG_HPP_

#include "AzOptOnTree.hpp"
#include "AzReg_TreeRegArr.hpp"

//! coordinate descent with regulatization using tree structure  
/*--------------------------------------------------------*/
class AzOptOnTree_TreeReg : /* extends */ public virtual AzOptOnTree
{
protected: 
  AzRgfTreeEnsemble *rgf_ens; 
  AzReg_TreeRegArr *reg_arr; 

public: 
  AzOptOnTree_TreeReg() : rgf_ens(NULL), reg_arr(NULL) {}
  void reset(AzReg_TreeRegArr *inp_reg_arr) {
    reg_arr = inp_reg_arr; 
  }

  virtual void optimize(AzRgfTreeEnsemble *ens, /* weights are updated */
                       const AzTrTreeFeat *tree_feat, 
                       int inp_ite_num=-1, 
                       double lam=-1, 
                       double sig=-1); 

  /*---  ---*/
  virtual void reset(const AzOptOnTree_TreeReg *inp) {
    AzOptOnTree::reset(inp); 
    rgf_ens = inp->rgf_ens; 
    reg_arr = inp->reg_arr; 
  }

protected: 
  //! override 
  virtual void update_with_features(double nlam, double nsig, double py_avg, 
                            AzRgf_forDelta *for_delta); 

  virtual void update_weight(int nx, 
                             int fx, 
                             double delta, 
                             AzReg_TreeReg *reg); 
  virtual double bestDelta(
                      int nx, 
                      int fx, 
                      AzReg_TreeReg *reg, 
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_delta) /* updated */
                      const; 
}; 

#endif 




