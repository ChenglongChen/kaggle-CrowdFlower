/* * * * *
 *  AzOptimizerT.hpp 
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

#ifndef _AZ_OPTIMIZER_T_HPP_
#define _AZ_OPTIMIZER_T_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"
#include "AzBmat.hpp"
#include "AzLoss.hpp"
#include "AzTrTreeFeat.hpp"
#include "AzTrTreeEnsemble_ReadOnly.hpp"
#include "AzRgfTreeEnsemble.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"

//! coordinate descent for weight optimization. 
/*--------------------------------------------------------*/
class AzOptimizerT
{
public: 
  virtual void reset(AzLossType loss_type, 
             const AzDvect *v_y, 
             const AzDvect *v_fixed_dw, /* user-assigned data point weights */
             const AzRegDepth *reg_depth, 
             AzParam &param, 
             bool beVerbose, 
             const AzOut out_req, 
             /*---  for warm start  ---*/
             const AzTrTreeEnsemble_ReadOnly *ens=NULL, 
             const AzTrTreeFeat *tree_feat=NULL, 
             const AzDvect *inp_v_p=NULL) = 0; 

  virtual void copyPred_to(AzDvect *out_v_p) const = 0; 

  virtual void resetPred(const AzBmat *m_tran, 
                         AzDvect *v_p) /* output */
                         const = 0; 
  virtual void optimize(AzRgfTreeEnsemble *ens, 
                       const AzTrTreeFeat *tree_feat, 
                       int inp_ite_num=-1, 
                       double lam=-1, 
                       double sig=-1) = 0; 
  virtual void optimize(AzRgfTreeEnsemble *ens, 
                       const AzTrTreeFeat *tree_feat, 
                       bool doRefreshP, 
                       int inp_ite_num=-1, 
                       double lam=-1, 
                       double sig=-1) {
    throw new AzException("AzOptimizerT::optimize(...,doRefreshP,...)", "No support"); 
  }
  virtual const AzDvect *weights() const = 0; 
  virtual double constant() const = 0; 
  virtual void printHelp(AzHelp &h) const = 0; 
}; 

#endif 
