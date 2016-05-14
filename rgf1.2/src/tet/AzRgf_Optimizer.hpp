/* * * * *
 *  AzRgf_Optimizer.hpp 
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

#ifndef _AZ_RGF_OPTIMIZER_HPP_
#define _AZ_RGF_OPTIMIZER_HPP_

#include "AzUtil.hpp"
#include "AzDmat.hpp"
#include "AzBmat.hpp"
#include "AzDataForTrTree.hpp"
#include "AzLoss.hpp"
#include "AzTrTreeEnsemble.hpp"
#include "AzRgfTreeEnsemble.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"
#include "AzHelp.hpp"

//! Abstract class: Weight optimizer interface.  
/*-------------------------------------------------------*/
class AzRgf_Optimizer
{
public:
  /*! Initialization */
  virtual void cold_start(AzLossType loss_type, 
             const AzDataForTrTree *training_data, /*!< training data */
             const AzRegDepth *reg_depth, /*!< regularizer using tree attributes */
             AzParam &param,  /*!< confignuration */
             const AzDvect *v_y, /*!< training targets */
             const AzDvect *v_fixed_dw, /* user-assigned data point weights */
             const AzOut out,    /*!< where to wrige log */
             /*! output: prediction on training data. typically zeroes. */
             AzDvect *v_p)
              = 0; 

  virtual void warm_start(AzLossType loss_type, 
             const AzDataForTrTree *training_data, /*!< training data */
             const AzRegDepth *reg_depth, /*!< regularizer using tree attributes */
             AzParam &param,  /*!< confignuration */
             const AzDvect *v_y, /*!< training targets */
             const AzDvect *v_fixed_dw, /* user-assigned data point weights */
             const AzOut out,    /*!< where to wrige log */
             /*---  for warm start  ---*/
             const AzTrTreeEnsemble_ReadOnly *inp_ens, 
             const AzDvect *inp_v_p)
              = 0; 

  /*! Optimize weights. */
  virtual void
  update(const AzDataForTrTree *data, /*!< training data */
         AzRgfTreeEnsemble *ens, /*!< inout: tree ensmeble. */
         /*---  output  ---*/
         AzDvect *v_p) /*<! predictions on training data */
         = 0;

  /*! Test */
  virtual void apply(const AzDataForTrTree *data, 
             AzBmat *b_test_tran, /*!< inout */
             const AzTrTreeEnsemble_ReadOnly *ens, 
             /*---  output  ---*/
             AzDvect *v_test_p, /*!< output: prediction on test data */
             int *f_num, /*!< output: number of features including removed ones */
             /*! output: number of features after consolidation and removing zero-weight ones */
             int *nz_f_num)
             const = 0;

  /*! simulate end of the training and return the test results */
  virtual void temp_update_apply(const AzDataForTrTree *tr_data, 
                          AzRgfTreeEnsemble *temp_ens, 
                          const AzDataForTrTree *test_data, 
                          AzBmat *temp_b, AzDvect *v_test_p, 
                          int *f_num, int *nz_f_num) const = 0; 

  virtual void printHelp(AzHelp &h) const = 0; 
}; 
#endif 

