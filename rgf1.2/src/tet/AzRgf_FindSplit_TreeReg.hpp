/* * * * *
 *  AzRgf_FindSplit_TreeReg.hpp 
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

#ifndef _AZ_RGF_FIND_SPLIT_TREE_REG_HPP_
#define _AZ_RGF_FIND_SPLIT_TREE_REG_HPP_

#include "AzRgf_FindSplit_Dflt.hpp"
#include "AzReg_TreeReg.hpp"
#include "AzReg_TreeRegArr.hpp"

//! Node split search for RGF.  L2 and tree structure regularization 
/*--------------------------------------------------------*/
class AzRgf_FindSplit_TreeReg : /* extends */  public virtual AzRgf_FindSplit_Dflt
{
protected:
  AzReg_TreeRegArr *reg_arr; 
  AzReg_TreeReg *reg; 
  double dR, ddR; 

public:
  AzRgf_FindSplit_TreeReg() : dR(0), ddR(0), reg(NULL), reg_arr(NULL) {}
  void reset(AzReg_TreeRegArr *inp_reg_arr) {
    reg_arr = inp_reg_arr; 
  }

  //! override 
  virtual void begin(const AzTrTree_ReadOnly *tree, 
                   const AzRgf_FindSplit_input &inp,  
                   int inp_min_size)
  {
    AzRgf_FindSplit_Dflt::begin(tree, inp, inp_min_size); 
    reg = reg_arr->reg_forNewLeaf(inp.tx); 
    reg->reset_forNewLeaf(tree, reg_depth); 
  }

  //! override 
  virtual void end() {
    AzRgf_FindSplit_Dflt::end(); 
    reg = NULL; 
  }

  //! override 
  virtual void findSplit(int nx, AzTrTsplit *best_split); 

  //! override AzFindSplit::evalSplit
  virtual double evalSplit(const Az_forFindSplit i[2], 
                           double bestP[2]) const; 
}; 
#endif 
