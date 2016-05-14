/* * * * *
 *  AzRgf_Optimizer_TreeReg.hpp 
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

#ifndef _AZ_RGF_OPTIMIZER_TREE_REG_HPP_
#define _AZ_RGF_OPTIMIZER_TREE_REG_HPP_

#include "AzRgf_Optimizer_Dflt.hpp"
#include "AzOptOnTree_TreeReg.hpp"

//! for regularization using tree structure. 
/*-------------------------------------------------------*/
class AzRgf_Optimizer_TreeReg : /* extends */ public virtual AzRgf_Optimizer_Dflt
{
protected:
  AzOptOnTree_TreeReg trainer_tr;

public: 
  AzRgf_Optimizer_TreeReg() {
    trainer = &trainer_tr; 
  }
  void reset(AzReg_TreeRegArr *reg_arr) {
    trainer_tr.reset(reg_arr); 
  }
  AzRgf_Optimizer_TreeReg(const AzRgf_Optimizer_TreeReg *inp) {
    reset(inp); 
  }

  /*------------------------------------------------------*/
  /* override this to replace trainer                     */
  /*------------------------------------------------------*/
  virtual void reset(const AzRgf_Optimizer_TreeReg *inp) {
    if (inp == NULL) return; 
    AzRgf_Optimizer_Dflt::reset(inp); 

    trainer_tr.reset(&inp->trainer_tr);  
    trainer = &trainer_tr; 
  }

  /*------------------------------------------------------*/
  /* derived classes must override this                   */
  /*------------------------------------------------------*/
  virtual void temp_update_apply(const AzDataForTrTree *tr_data, 
                          AzRgfTreeEnsemble *temp_ens, 
                          const AzDataForTrTree *test_data, 
                          AzBmat *temp_b, AzDvect *v_test_p, 
                          int *f_num, int *nz_f_num) const {   
    AzRgf_Optimizer_TreeReg temp_opt(this);   
    temp_opt.update(tr_data, temp_ens); 
    if (test_data != NULL) temp_opt.apply(test_data, temp_b, temp_ens, 
                                          v_test_p, f_num, nz_f_num); 
  }
  /*--------------------------------------------------------*/
}; 
#endif 





