/* * * * *
 *  AzRgf_Optimizer_Dflt.hpp 
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

#ifndef _AZ_RGF_OPTIMIZER_DFLT_HPP_
#define _AZ_RGF_OPTIMIZER_DFLT_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"
#include "AzDataForTrTree.hpp"
#include "AzRgfTree.hpp"
#include "AzTrTreeEnsemble.hpp"
#include "AzLoss.hpp"
#include "AzTrTreeFeat.hpp"
#include "AzOptOnTree.hpp"
#include "AzRgf_Optimizer.hpp"
#include "AzOptimizerT.hpp"

//! implement AzRgf_Optimizer. 
/*-------------------------------------------------------*/
class AzRgf_Optimizer_Dflt : /* implements */ public virtual AzRgf_Optimizer
{
protected:
  AzTrTreeFeat  feat1; 
  AzOut out; 

  AzOptOnTree trainer_dflt; /* linear trainer */
  AzOptimizerT *trainer; 

public: 
  AzRgf_Optimizer_Dflt() : trainer(&trainer_dflt) {}
  ~AzRgf_Optimizer_Dflt() {}
  AzRgf_Optimizer_Dflt(const AzRgf_Optimizer_Dflt *inp) {
    reset(inp); 
  }

  /*------------------------------------------------------*/
  /* override this to replace trainer                     */
  /*------------------------------------------------------*/
  virtual void reset(const AzRgf_Optimizer_Dflt *inp) {
    if (inp == NULL) return; 
    feat1.reset(&inp->feat1); 
    out = inp->out;   
    trainer_dflt.reset(&inp->trainer_dflt);  
    trainer = &trainer_dflt; 
  }
  /*------------------------------------------------------*/

  /*------------------------------------------------------*/
  /* derived classes must override this                   */
  /*------------------------------------------------------*/
  virtual void temp_update_apply(const AzDataForTrTree *tr_data, 
                          AzRgfTreeEnsemble *temp_ens, 
                          const AzDataForTrTree *test_data, 
                          AzBmat *temp_b, AzDvect *v_test_p, 
                          int *f_num, int *nz_f_num) const {   
    AzRgf_Optimizer_Dflt temp_opt(this);   
    temp_opt.update(tr_data, temp_ens); 
    if (test_data != NULL) temp_opt.apply(test_data, temp_b, temp_ens, 
                                          v_test_p, f_num, nz_f_num); 
  }
  /*--------------------------------------------------------*/


  virtual void cold_start(AzLossType loss_type, 
             const AzDataForTrTree *data, 
             const AzRegDepth *reg_depth, 
             AzParam &param, 
             const AzDvect *v_yval, 
             const AzDvect *v_fixed_dw, /* user-assigned data point weights */
             const AzOut out_req, 
             AzDvect *v_pval); /* output */
  virtual void warm_start(AzLossType loss_type, 
                     const AzDataForTrTree *data, 
                     const AzRegDepth *reg_depth, 
                     AzParam &param, 
                     const AzDvect *v_y, /* for training */
                     const AzDvect *v_fixed_dw, /* user-assigned data point weights */
                     const AzOut out_req, 
                     /*---  for warm start  ---*/
                     const AzTrTreeEnsemble_ReadOnly *ens, 
                     const AzDvect *v_p); 
  virtual void 
  update(const AzDataForTrTree *data,                
                  AzRgfTreeEnsemble *ens, /* inout */
                  /*---  output  ---*/
                  AzDvect *v_p=NULL); /* prediction */

  virtual void apply(const AzDataForTrTree *data, 
             AzBmat *b_test_tran, /* inout */
             const AzTrTreeEnsemble_ReadOnly *ens, 
             /*---  output  ---*/
             AzDvect *v_p, 
             int *f_num, int *nz_f_num) const; 

  virtual void printHelp(AzHelp &h) const; 

protected:
  virtual bool resetParam(AzParam &param); 
  static void _info(const AzTrTreeEnsemble_ReadOnly *ens, 
                    const AzOptimizerT *my_trainer, 
                    const AzTrTreeFeat *my_feat, 
                    int *f_num, int *nz_f_num); 
}; 
#endif 
