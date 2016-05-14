/* * * * *
 *  AzTET_Eval.hpp 
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

#ifndef _AZ_TET_EVAL_HPP_
#define _AZ_TET_EVAL_HPP_

#include "AzDataForTrTree.hpp"
#include "AzLoss.hpp"
#include "AzTE_ModelInfo.hpp"
#include "AzPerfResult.hpp"

//! Abstract class: interface for evaluation modules for Tree Ensemble Trainer.  
/*-------------------------------------------------------*/
class AzTET_Eval {
public: 
  virtual void reset(const AzDvect *inp_v_y, 
                     const char *perf_fn, 
                     bool inp_doAppend) = 0; 
  virtual void begin(const char *config="", 
                     AzLossType loss_type=AzLoss_None) = 0; 
  virtual void resetConfig(const char *config) = 0; 
  virtual void end() = 0; 
  virtual void evaluate(const AzDvect *v_p, const AzTE_ModelInfo *info, 
                        const char *user_str=NULL) = 0; 
  virtual bool isActive() const = 0; 
}; 
#endif 
