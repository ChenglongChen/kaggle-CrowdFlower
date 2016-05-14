/* * * * *
 *  AzRgforest.hpp 
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

#ifndef _AZ_RGFOREST_HPP_
#define _AZ_RGFOREST_HPP_

#include <ctime>

#include "AzUtil.hpp"
#include "AzRgfTree.hpp"
#include "AzLoss.hpp"
#include "AzTETrainer.hpp"
#include "AzTrTtarget.hpp"
#include "AzTimer.hpp"

#include "AzRgf_Optimizer.hpp"
#include "AzRgf_Optimizer_Dflt.hpp"
#include "AzRgf_FindSplit_Dflt.hpp"
#include "AzRgfTreeEnsImp.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"

//! RGF main.  
class AzRgforest : /* implements */ public virtual AzTETrainer {
protected: 
  AzBytArr s_config; 

  AzRgfTreeEnsImp<AzRgfTree> dflt_ens; 
  AzRgfTreeEnsemble *ens; 
  AzRgf_FindSplit_Dflt dflt_fs; 
  AzRgf_FindSplit *fs; 
  AzRgf_Optimizer_Dflt dflt_opt;  /* weight optimizer */
  AzRgf_Optimizer *opt;
  AzRegDepth dflt_reg_depth;  
  AzRegDepth *reg_depth; 

  AzDataForTrTree dflt_data; 
  const AzDataForTrTree *data; /* This should be set in setInput */
  
  AzTrTtarget target; 

  bool isOpt; 

  int rootonly_tx; 
  AzRgfTree dflt_tree; 
  AzRgfTree *rootonly_tree; 

  /*---  parameters  ---*/
  AzLossType loss_type; 
  bool doForceToRefreshAll; /* for debug */
  int s_tree_num;    /* # of latest trees to be searched */
  bool beVerbose; 
  AzBytArr s_mem_policy; 
  bool beTight;
  AzBytArr s_temp_for_trees; 
  double f_ratio; 
  int f_pick; 
  bool doPassiveRoot; 

  /*---  work area  ---*/
  int l_num; 
  double py_adjust, lam_scale; /* for numerical stability for exp loss */
  AzDvect v_p; /* prediction */
  AzTimer test_timer, opt_timer, lmax_timer; 
  AzOut out; 

  bool doTime; 
  clock_t opt_time, search_time; 

  static const int lnum_inc_opt_dflt = 100; 
  static const int max_lnum_dflt = 10000; 
  static const int lnum_inc_test_dflt = 500; 
  static const int s_tree_num_dflt = 1; 
  static const AzLossType loss_type_dflt = AzLoss_Square; 

public:
  AzRgforest() : 
    rootonly_tx(-1), data(NULL),   
    s_tree_num(s_tree_num_dflt), 
    loss_type(loss_type_dflt), 
    doForceToRefreshAll(false), beVerbose(false),  
    l_num(0), isOpt(false), out(log_out), py_adjust(0), lam_scale(1), 
    opt_time(0), search_time(0), doTime(false), 
    beTight(false), s_mem_policy(mp_not_beTight), 
    f_ratio(-1), f_pick(-1), 
    doPassiveRoot(false) 
  {
    opt = &dflt_opt; 
    ens = &dflt_ens; 
    fs = &dflt_fs;
    /* data = &dflt_data; should be set in setInput */
    rootonly_tree = &dflt_tree; 
    reg_depth = &dflt_reg_depth; 
  }
  ~AzRgforest() {}

  virtual inline const char *signature() const {
    return "-___-_RGF_"; 
  }

  virtual 
  void startup(const AzOut &out, 
              const char *param, 
              AzSmat *m_x, 
              AzDvect *v_y, 
              const AzSvFeatInfo *featInfo=NULL, 
              AzDvect *v_fixed_dw=NULL, 
              AzTreeEnsemble *inp_ens=NULL) /* may be NULL */
  {
    check_data_consistency(m_x, v_y, v_fixed_dw, featInfo, "AzRgforest::startup"); 

    if (inp_ens == NULL) cold_start(param, m_x, v_y, featInfo, v_fixed_dw, out); 
    else                 warm_start(param, m_x, v_y, featInfo, v_fixed_dw, inp_ens, out); 
    m_x->destroy(); 
    v_y->destroy(); 
    if (v_fixed_dw != NULL) v_fixed_dw->destroy(); 
    if (inp_ens != NULL) inp_ens->destroy(); 
  }
  virtual AzTETrainer_Ret proceed_until(); 

  virtual void  
  apply(AzTETrainer_TestData *td, 
        /*---  output  ---*/
        AzDvect *v_test_p,
        AzTE_ModelInfo *info=NULL, 
        AzTreeEnsemble *out_ens=NULL) 
        const;

  AzLossType lossType() const {
    return loss_type; 
  }

  virtual 
  void copy_to(AzTreeEnsemble *out_ens) const; 

  virtual const char *description() const {
    return "Regularized greedy forest"; 
  }

protected:
  /*----------------------------------------------------------------*/
  /* override this if replacing trees and if that affects optimizer */
  /*----------------------------------------------------------------*/
  virtual void temp_apply_copy_to(AzTreeEnsemble *out_ens, 
                          const AzDataForTrTree *test_data, 
                          AzBmat *temp_b, AzDvect *v_test_p, 
                          int *f_num, int *nz_f_num) const {
    AzRgfTreeEnsImp<AzRgfTree> temp_ens; 
    temp_ens.copy_nodes_from(ens);
    opt->temp_update_apply(data, &temp_ens, test_data, temp_b, 
                           v_test_p, f_num, nz_f_num); 
    if (out_ens != NULL) temp_ens.copy_to(out_ens, s_config.c_str(), signature()); 
  }
  /*----------------------------------------------------------------*/

  virtual void setInput(AzParam &p, 
                        const AzSmat *m_x, 
                        const AzSvFeatInfo *featInfo); 
  virtual void initEnsemble(AzParam &param, int max_tree_num); 

  virtual bool growForest(); 
  AzRgfTree *tree_to_grow(int &best_tx,  /* inout */
                             int &best_nx,  /* inout */
                             bool *isNewTree); /* output */

  virtual const AzRgfTree *splitNode(AzTrTsplit *best_split, 
                                     double *w_inc, /* output */
                                     int leaf_nx[2]); /* output */

  /*---  for weight optimization/correction  ---*/
  virtual void optimize_resetTarget();

  /*---  for updating targets  ---*/
  virtual void initTarget(const AzDvect *v_y, 
                          const AzDvect *v_fixed_dw); 
  virtual void resetTarget();          
  virtual inline void updateTarget(const AzRgfTree *tree, 
                                   const int leaf_nx[2], 
                                   double w_inc) 
    /* w_inc: increase in the weight of the node we just split */
    /*        0 if using internal nodes;                       */
  {
    if (loss_type == AzLoss_Square) {
      _updateTarget_LS(tree, leaf_nx, w_inc, &target, &v_p); 
    }
    else {
     _updateTarget_OtherLoss(tree, leaf_nx, w_inc); 
    }
  }
  static void _updateTarget_LS(const AzRgfTree *tree, 
                               const int leaf_nx[2], 
                               double w_inc, 
                               AzTrTtarget *target, 
                               AzDvect *v_p); 
  virtual void _updateTarget_OtherLoss(const AzRgfTree *tree, 
                                       const int leaf_nx[2], 
                                       double w_inc); 

  /*---  for search  ---*/
  virtual void searchBestSplit(AzTrTsplit *best_split); 

  /*----*/
  bool shouldExit(const AzTrTsplit *best_split) const; 

  virtual void printHelp(AzHelp &h) const; 
  virtual int resetParam(AzParam &param); /* returns max #tree */
  int adjustTestInterval(int lnum_inc_test, int lnum_inc_opt); 

  virtual void show_tree_info() const; 
  virtual void show_forExpoFamily(const AzDvect *v_dw) const; 

  /*! place holder for extension; called at the end of initialization  */
  virtual void end_of_initialization() {
    if (target.isWeighted()) {
      AzPrint::writeln(out, "--------------------"); 
      AzPrint::writeln(out, "Sum of data point weights = ", target.sum_fixed_dw()); 
    }
    AzPrint::writeln(out, "--------------------"); 
  } 

  /*! place holder for extension; called at the end of training */
  virtual inline void end_of_training() {}

  /*---  for time measurement  ---*/
  inline virtual void time_init() {
    opt_time = search_time = 0; 
  }
  inline virtual void time_begin(clock_t *b_time) /* output */ {
    if (doTime) *b_time = clock(); 
  }
  inline virtual void time_end(clock_t b_time, /* input */
                               clock_t *accum_time) /* inout */ {
    if (doTime) *accum_time += (clock() - b_time); 
  }
  virtual void time_show(); 

  virtual void cold_start(const char *param, 
              const AzSmat *m_x, 
              const AzDvect *v_y, 
              const AzSvFeatInfo *featInfo, 
              const AzDvect *v_fixed_dw, 
              const AzOut &out); 
  virtual void warm_start(const char *param,
              const AzSmat *m_x,  
              const AzDvect *v_y, 
              const AzSvFeatInfo *featInfo, 
              const AzDvect *v_fixed_dw, 
              const AzTreeEnsemble *inp_ens, 
              const AzOut &out); 

  virtual void warmup_timer(const AzTreeEnsemble *inp_ens, 
                            int max_tree_num); 

  virtual void warmupEnsemble(AzParam &az_param, int max_tree_num, 
                              const AzTreeEnsemble *inp_ens); 
}; 
#endif
