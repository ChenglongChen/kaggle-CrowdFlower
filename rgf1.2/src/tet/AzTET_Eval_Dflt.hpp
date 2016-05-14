/* * * * *
 *  AzTET_Eval_Dflt.hpp 
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

#ifndef _AZ_TET_EVAL_DFLT_HPP_
#define _AZ_TET_EVAL_DFLT_HPP_

#include "AzUtil.hpp"
#include "AzTaskTools.hpp"
#include "AzPerfResult.hpp"
#include "AzDataForTrTree.hpp"
#include "AzLoss.hpp"
#include "AzTET_Eval.hpp"

//! Evaluationt module for Tree Ensemble Trainer.  
/*-------------------------------------------------------*/
class AzTET_Eval_Dflt : /* implements */ public virtual AzTET_Eval {
protected:
  /*---  targets  ---*/
  const AzDvect *v_y;

  /*---  to output evaluation results  ---*/
  AzBytArr s_perf_fn; 
  AzPerfType perf_type; 

  AzLossType loss_type; 
  AzBytArr s_config; 

  AzOfs ofs; 
  AzOut perf_out; 
  bool doAppend; 

public: 
  AzTET_Eval_Dflt() :  v_y(NULL), 
                loss_type(AzLoss_None), doAppend(false) {}
  ~AzTET_Eval_Dflt() {
    end(); 
  }
  inline virtual bool isActive() const {
    if (v_y != NULL) return true; 
    return false; 
  }

  virtual void reset() {
    v_y= NULL; 
    s_perf_fn.reset(); 
    s_config.reset(); 
    if (ofs.is_open()) {
      ofs.close(); 
    }
  }
  void reset(const AzDvect *inp_v_y, 
             const char *perf_fn, 
             bool inp_doAppend) 
  {
    v_y = inp_v_y;  
    s_perf_fn.reset(perf_fn); 
    doAppend = inp_doAppend; 
  }
  virtual void resetConfig(const char *config) {
    s_config.reset(config); 
    _clean(&s_config); 
  }

  virtual void begin(const char *config, 
                     AzLossType inp_loss_type) {
    if (!isActive()) return; 
    _begin(config, inp_loss_type); 
    _clean(&s_config); 
  }
  virtual void end() {
    if (ofs.is_open()) {
      ofs.close(); 
    }
  }

  virtual void evaluate(const AzDvect *v_p, 
                       const AzTE_ModelInfo *info, 
                       const char *user_str=NULL) {
    if (!isActive()) return; 
    AzPerfResult result=AzTaskTools::eval(v_p, v_y, loss_type); 

    /*---  signature and configuration  ---*/
    AzBytArr s_sign_config(info->s_sign); 
    s_sign_config.concat(":"); 
    concat_config(info, &s_sign_config); 

    /*---  print  ---*/
    AzPrint o(perf_out); 
    o.printBegin("", ",", ","); 
    o.print("#tree", info->tree_num); 
    o.print("#leaf", info->leaf_num); 
    o.print("acc", result.acc, 4); 
    o.print("rmse", result.rmse, 4); 
    o.print("sqerr", result.rmse*result.rmse, 6); 
    o.print(loss_str[loss_type]); 
    o.print("loss", result.loss, 6); 
    o.print("#test", v_p->rowNum()); 
    o.print("cfg"); /* for compatibility */
    o.print(s_sign_config); 
    if (user_str != NULL) {
      o.print(user_str); 
    }
    o.printEnd();
  }

protected:
  virtual void _begin(const char *config, AzLossType inp_loss_type) {
    s_config.reset(config); 
    loss_type = inp_loss_type; 

    if (ofs.is_open()) {
      ofs.close(); 
    }
    const char *perf_fn = s_perf_fn.c_str(); 
    if (AzTools::isSpecified(perf_fn)) {
      ios_base::openmode mode = ios_base::out; 
      if (doAppend) {
        mode = ios_base::app | ios_base::out; 
      }
      ofs.open(perf_fn, mode); 
      ofs.set_to(perf_out); 
    }
    else {
      perf_out.setStdout(); 
    }
  }

  virtual void _clean(AzBytArr *s) const {
    /*--  replace comma with : for convenience later --*/
    s->replace(',', ';'); 
  }

  virtual void concat_config(const AzTE_ModelInfo *info, AzBytArr *s) const {
    if (s_config.length() > 0) {
      s->concat(&s_config); 
    }
    else {
      AzBytArr s_cfg(&info->s_config); 
      _clean(&s_cfg); 
      s->concat(&s_cfg); 
    }
  }
}; 
#endif 
