/* * * * *
 *  AzRgforest.cpp 
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

#include "AzRgforest.hpp"
#include "AzHelp.hpp"
#include "AzRgf_kw.hpp"

/*-------------------------------------------------------------------*/
void AzRgforest::cold_start(const char *param, 
                        const AzSmat *m_x, 
                        const AzDvect *v_y, 
                        const AzSvFeatInfo *featInfo, 
                        const AzDvect *v_fixed_dw, 
                        const AzOut &out_req)
{
  out = out_req; 
  s_config.reset(param); 

  AzParam az_param(param); 
  int max_tree_num = resetParam(az_param); 
  setInput(az_param, m_x, featInfo);        
  reg_depth->reset(az_param, out);  /* init regularizer on node depth */
  v_p.reform(v_y->rowNum()); 
  opt->cold_start(loss_type, data, reg_depth, /* initialize optimizer */
                  az_param, v_y, v_fixed_dw, out, &v_p); 
  initTarget(v_y, v_fixed_dw);    
  initEnsemble(az_param, max_tree_num); /* initialize tree ensemble */
  fs->reset(az_param, reg_depth, out); /* initialize node search */
  az_param.check(out); 
  l_num = 0; /* initialize leaf node counter */

  if (!beVerbose) { 
    out.deactivate(); /* shut up after printing everyone's config */
  }

  time_init(); /* initialize time measure ment */
  end_of_initialization(); 
}

/*-------------------------------------------------------------------*/
void AzRgforest::warm_start(const char *param, 
                        const AzSmat *m_x, 
                        const AzDvect *v_y, 
                        const AzSvFeatInfo *featInfo, 
                        const AzDvect *v_fixed_dw, 
                        const AzTreeEnsemble *inp_ens, 
                        const AzOut &out_req)
{
  const char *eyec = "AzRgforest::warm_start"; 
  out = out_req; 
  if (inp_ens->orgdim() > 0 && 
      inp_ens->orgdim() != m_x->rowNum()) {
    AzBytArr s("Mismatch in feature dimensionality.  "); 
    s.cn(inp_ens->orgdim());s.c(" (tree ensemeble), ");s.cn(m_x->rowNum());s.c(" (dataset)."); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }
  s_config.reset(param); 

  AzParam az_param(param); 
  int max_tree_num = resetParam(az_param); 

  warmup_timer(inp_ens, max_tree_num); /* timers are modified for warm-start */

  setInput(az_param, m_x, featInfo); 

  AzTimeLog::print("Warming-up trees ... ", log_out); 
  warmupEnsemble(az_param, max_tree_num, inp_ens); /* v_p is set */

  reg_depth->reset(az_param, out);  /* init regularizer on node depth */

  AzTimeLog::print("Warming-up the optimizer ... ", log_out); 
  opt->warm_start(loss_type, data, reg_depth, /* initialize optimizer */
                  az_param, v_y, v_fixed_dw, out, 
                  ens, &v_p); 

  initTarget(v_y, v_fixed_dw); 

  fs->reset(az_param, reg_depth, out); /* initialize node search */
  az_param.check(out); 
  l_num = ens->leafNum();  /* warm-up #leaf */

  if (!beVerbose) { 
    out.deactivate(); /* shut up after printing everyone's config */
  }

  time_init(); /* initialize time measure ment */
  end_of_initialization(); 
  AzTimeLog::print("End of warming-up ... ", log_out); 
}

/*------------------------------------------------------------------*/
/* Update test_timer, opt_timer */
void AzRgforest::warmup_timer(const AzTreeEnsemble *inp_ens, 
                              int max_tree_num)
{
  const char *eyec = "AzRgforest::warmup_timer"; 
  /*---  check consistency and adjust intervals for warm start ---*/
  int inp_leaf_num = inp_ens->leafNum(); 
  if (lmax_timer.reachedMax(inp_leaf_num) || 
      max_tree_num < inp_ens->size()) {
    AzBytArr s("The model given for warm-start is already over "); 
    s.concat("the requested maximum size of the models: #leaf="); 
    s.cn(inp_leaf_num); s.c(", #tree="); s.cn(inp_ens->size()); 
    throw new AzException(AzInputError, eyec, s.c_str()); 
  }

  int inp_leaf_num_test = inp_leaf_num; 
  if (inp_leaf_num_test % test_timer.inc == 1) --inp_leaf_num_test; 
  test_timer.chk += inp_leaf_num_test; 

  int inp_leaf_num_opt = inp_leaf_num; 
  if (inp_leaf_num_opt % opt_timer.inc == 1) --inp_leaf_num_opt; 
  opt_timer.chk += inp_leaf_num_opt; 
}

/*------------------------------------------------------------------*/
/* output: ens, output: v_p */
void AzRgforest::warmupEnsemble(AzParam &az_param, int max_tree_num, 
                                const AzTreeEnsemble *inp_ens)
{
  ens->warm_start(inp_ens, data, az_param, &s_temp_for_trees, 
                  out, max_tree_num, s_tree_num, &v_p); 

  /*---  always have one unsplit root: represent the next tree  ---*/
  rootonly_tree->reset(az_param); 
  rootonly_tree->makeRoot(data); 

  rootonly_tx = max_tree_num + 1;  /* any number that doesn't overlap other trees */
}

/*-------------------------------------------------------------------*/
/* input: v_p */
void AzRgforest::initTarget(const AzDvect *v_y, 
                            const AzDvect *v_fixed_dw)
{
  target.reset(v_y, v_fixed_dw);
  resetTarget(); 
}

/*-------------------------------------------------------------------*/
void AzRgforest::setInput(AzParam &p, 
                          const AzSmat *m_x, 
                          const AzSvFeatInfo *featInfo)
{
  dflt_data.reset_data(out, m_x, p, beTight, featInfo); 
  data = &dflt_data; 

  f_pick = -1; 
  if (f_ratio > 0) {
    f_pick = (int)((double)data->featNum() * f_ratio); 
    f_pick = MAX(1, f_pick); 
    AzPrint::writeln(out, "#feature to be sampled = ", f_pick); 
  }
}

/*------------------------------------------------------------------*/
void AzRgforest::initEnsemble(AzParam &az_param, int max_tree_num)
{
  const char *eyec = "AzRgforest::initEnsemble"; 

  if (max_tree_num < 1) {
    throw new AzException(AzInputNotValid, "AzRgforest::initEnsemble", 
                          "max# must be positive"); 
  }

  ens->cold_start(az_param, &s_temp_for_trees, data->dataNum(), 
                  out, max_tree_num, data->featNum()); 

  /*---  always have one unsplit root: represent the next tree  ---*/
  rootonly_tree->reset(az_param); 
  rootonly_tree->makeRoot(data); 

  rootonly_tx = max_tree_num + 1;  /* any number that doesn't overlap other trees */
}

/*-------------------------------------------------------------------*/
AzRgfTree *AzRgforest::tree_to_grow(int &best_tx,  /* inout */
                                   int &best_nx,  /* inout */
                                   bool *isNewTree) /* output */
{
  if (best_tx != rootonly_tx) {
    *isNewTree = false; 
    return ens->tree_u(best_tx); 
  }
  else {
    AzRgfTree *tree = ens->new_tree(&best_tx); /* best_tx is updated */
    tree->reset(out); 
    best_nx = tree->makeRoot(data); 
    *isNewTree = true; 
    return tree; 
  }
}

/*-------------------------------------------------------------------*/
AzTETrainer_Ret AzRgforest::proceed_until()
{
  AzTETrainer_Ret ret = AzTETrainer_Ret_Exit; 
  for ( ; ; ) {
    /*---  grow the forest  ---*/
    bool doExit = growForest(); 
    if (doExit) break; 

    /*---  optimize weights  ---*/
    if (opt_timer.ringing(false, l_num)) {
      optimize_resetTarget(); 
      show_tree_info(); 
    }

    /*---  time to test?  ---*/
    bool doTestNow = test_timer.ringing(false, l_num); 
    if (doTestNow) {  
      ret = AzTETrainer_Ret_TestNow; 
      break; /* time to test */
    }
  }

  if (ret == AzTETrainer_Ret_Exit) {
    if (!isOpt) {
      optimize_resetTarget(); 
    }
    time_show(); 
    end_of_training(); 
  }

  return ret; 
}

/*------------------------------------------------------------------*/
void AzRgforest::time_show()
{
  if (doTime) {
    AzOut my_out = out; 
    my_out.activate(); 
    if (my_out.isNull()) return; 
    AzPrint o(my_out); 
    o.printBegin("", ", ", "="); 
    o.print("search_time", (double)(search_time/(double)CLOCKS_PER_SEC)); 
    o.print("opt_time", (double)(opt_time/(double)CLOCKS_PER_SEC)); 
    o.printEnd(); 
  }
}

/*------------------------------------------------------------------*/
bool AzRgforest::growForest()
{
  clock_t b_time; 
  time_begin(&b_time); 

  /*---  find the best split  ---*/
  AzTrTsplit best_split; 
  searchBestSplit(&best_split);                    
  if (shouldExit(&best_split)) { /* exit if no more split */
    return true; /* exit */
  }

  /*---  split the node  ---*/
  double w_inc; 
  int leaf_nx[2] = {-1,-1}; 
  const AzRgfTree *tree = splitNode(&best_split, &w_inc, leaf_nx); 

  if (lmax_timer.reachedMax(l_num, "AzRgforest: #leaf", out)) { 
    return true; /* #leaf reached max; exit */
  }

  /*---  update target  ---*/
  updateTarget(tree, leaf_nx, w_inc); 

  time_end(b_time, &search_time); 
  return false; /* don't exit */
}

/* changes: isOpt, l_num */
/*------------------------------------------------------------------*/
const AzRgfTree *AzRgforest::splitNode(AzTrTsplit *best_split, /* (tx,nx) may be updated */
                             /*---  output  ---*/
                             double *w_inc, 
                             int leaf_nx[2])
{                             
  bool isNewTree = false; 
  AzRgfTree *tree = tree_to_grow(best_split->tx, best_split->nx, &isNewTree); 
  double old_w = tree->node(best_split->nx)->weight; 
  tree->splitNode(data, best_split); 
  double new_w = tree->node(best_split->nx)->weight; 
  isOpt = false; 

  ++l_num; /* if it wasn't root, one leaf was removed and two leaves were added */
  if (best_split->nx == tree->root()) {
    ++l_num; 
  }

  *w_inc = new_w - old_w; 

  leaf_nx[0] = tree->node(best_split->nx)->le_nx; 
  leaf_nx[1] = tree->node(best_split->nx)->gt_nx; 

  return tree; 
}

/*------------------------------------------------------------------*/
bool AzRgforest::shouldExit(const AzTrTsplit *best_split) const
{
  /*---  exit criteria  ---*/
  if (best_split->tx < 0 || best_split->fx < 0) {
    AzTimeLog::print("AzRgforest::shouldExit, No more split", out); 
    return true; 
  }
  
  if (best_split->tx == rootonly_tx && 
      ens->isFull()) {
    AzTimeLog::print("AzRgforest::shouldExit, #tree reached max ... ", out); 
    return true; 
  }
  return false; 
}

/*------------------------------------------------------------------*/
void AzRgforest::searchBestSplit(AzTrTsplit *best_split) /* must be initialize by caller */
{
  bool doRefreshAll = false; 
  if (doForceToRefreshAll) { /* this option is for testing the code */
    doRefreshAll = true; 
  }
  if (s_tree_num > 1) {
    doRefreshAll = true; 
  }

  int last_tx = ens->lastIndex(); 

  /*---  decide which trees should be searched  ---*/
  int my_first = MAX(0, last_tx + 1 - s_tree_num); 
  if (my_first-1 >= 0) { 
    /* 
     * since tree[my_first-1] will never be searched again, release some memory. 
     * split info may've been already removed, but that's okay 
     */
    ens->tree_u(my_first-1)->releaseWork();
  }

  /*---  search!  ---*/
  double nn = data->dataNum(); 
  const AzTrTtarget *tar = &target; 
  AzTrTtarget my_tar; 
  if (target.isWeighted()) {
    my_tar.reset(&target);
    my_tar.weight_tarDw(); 
    my_tar.weight_dw(); 
    nn = target.sum_fixed_dw(); 
    tar = &my_tar; 
  }

  if (f_pick > 0) {
    fs->pickFeats(f_pick, data->featNum()); 
  }

  AzRgf_FindSplit_input input(-1, data, tar, lam_scale, nn); 
  int tx; 
  for (tx = my_first; tx <= last_tx; ++tx) {
    input.tx = tx; 
    ens->tree_u(tx)->findSplit(fs, input, doRefreshAll, best_split);
  }
  /*---  rootonly tree  ---*/
  if (!doPassiveRoot || 
      best_split->tx < 0 || best_split->fx < 0) {
    input.tx = rootonly_tx; 
    rootonly_tree->findSplit(fs, input, doRefreshAll, best_split); 
  }
}

/*------------------------------------------------------------------*/
/* print this to stdout only when Dump is specified */
void AzRgforest::show_tree_info() const
{
  if (dmp_out.isNull()) return; 
  if (out.isNull()) return; 
  int t_num = ens->size(); 
  double max_max_depth = 0, avg_max_depth = 0; 
  double max_size = 0, avg_size = 0; 
  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    const AzTrTree_ReadOnly *tree = ens->tree(tx); 
    int max_depth = tree->maxDepth(); 
    max_max_depth = MAX(max_max_depth, max_depth); 
    avg_max_depth += max_depth; 
    int tree_size = tree->leafNum(); 
    max_size = MAX(max_size, tree_size); 
    avg_size += tree_size; 
  }
  if (t_num > 0) {
    avg_max_depth /= (double)t_num; 
    avg_size /= (double)t_num; 
  }

  AzPrint o(out); 
  o.printBegin("", ",", "="); 
  o.print("#tree", t_num); 
  o.print("max_depth-max,avg"); 
  o.pair_inParen(max_max_depth, avg_max_depth, ","); 
  o.print("#leaf-max,avg"); 
  o.pair_inParen(max_size, avg_size, ",");  
  o.printEnd(); 
}

/*------------------------------------------------------------------*/
void AzRgforest::optimize_resetTarget()
{
  clock_t b_time; 
  time_begin(&b_time); 

  int t_num = ens->size(); 

  AzBytArr s("Calling optimizer with "); 
  s.cn(t_num); s.c(" trees and "); s.cn(l_num); s.c(" leaves"); 
  AzTimeLog::print(s, out); 

  opt->update(data, ens, &v_p); 
  resetTarget(); 

  int tx; 
  for (tx = 0; tx < t_num; ++tx) {
    ens->tree_u(tx)->removeSplitAssessment(); /* since weights changed */  
  }

  isOpt = true; 

  time_end(b_time, &opt_time); 
}

/* updates target and v_p */
/*------------------------------------------------------------------*/
void AzRgforest::_updateTarget_OtherLoss(const AzRgfTree *tree, 
                                         const int leaf_nx[2], 
                                         double w_inc)
{
  const double *y = target.y()->point(); 
  double *p = v_p.point_u(); 

  double *tar_dw = target.tarDw_forUpdate()->point_u(); 
  double *dw = target.dw_forUpdate()->point_u(); 

  int kx; 
  for (kx = 0; kx < 2; ++kx) {
    const AzTrTreeNode *np = tree->node(leaf_nx[kx]); 
    int num = np->dxs_num; 
    const int *dxs = np->data_indexes(); 
    double new_w = np->weight; 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int dx = dxs[ix]; 

      p[dx] += (new_w + w_inc); 

      AzLosses o = AzLoss::getLosses(loss_type, p[dx], y[dx], py_adjust); 
      dw[dx] = o.loss2; 
      tar_dw[dx] = o._loss1;  
    }
  }
}

/*------------------------------------------------------------------*/
/* static */
//! _updataeTarget_OtherLoss could be called for LS too.  
//  But perhaps this is faster.    
void AzRgforest::_updateTarget_LS(const AzRgfTree *tree, 
                                  const int leaf_nx[2], 
                                  double w_inc, 
                                  AzTrTtarget *target, /* updated */
                                  AzDvect *v_p)        /* updated */
{
  double *r = target->tarDw_forUpdate()->point_u(); 
  double *p = v_p->point_u(); 

  int kx; 
  for (kx = 0; kx < 2; ++kx) {
    const AzTrTreeNode *np = tree->node(leaf_nx[kx]); 
    int num = np->dxs_num; 
    const int *dxs = np->data_indexes(); 
    double new_w = np->weight; 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int dx = dxs[ix]; 
      p[dx] += new_w; 
      r[dx] -= new_w; 
      if (w_inc != 0) {
        p[dx] += w_inc; 
        r[dx] -= w_inc; 
      }
    }
  }
}

/* Input: y, p */
/* Output: tar, dw, loss */
/*------------------------------------------------------------------*/
void AzRgforest::resetTarget()
{
  if (loss_type == AzLoss_Square) {
    target.resetTarDw_residual(&v_p); 
    return; 
  }

  AzDvect *v_tar_dw = target.tarDw_forUpdate();  /* tar*dw */
  AzDvect *v_dw = target.dw_forUpdate(); 

  /*---  train for -L'/L''  ---*/
  lam_scale = 
  AzLoss::negativeDeriv12(loss_type, &v_p, target.y(), NULL, 
                          &py_adjust, 
                          v_tar_dw, /* -L' */
                          v_dw);  /* L'' */

  if (!out.isNull() && AzLoss::isExpoFamily(loss_type)) {
    show_forExpoFamily(v_dw); 
  }
}

/*-------------------------------------------------------------------*/
/* print to stdout only when Dump is specified */
void AzRgforest::show_forExpoFamily(const AzDvect *v_dw) const
{
  if (dmp_out.isNull()) return; 
  if (out.isNull()) return; 
  double w_sum = v_dw->sum(); 
  double max_w = v_dw->max(); 
  double min_w = v_dw->min(); 
  AzPrint o(out); 
  o.reset_options(); 
  o.printBegin("resetTarget", ","); 
  o.printV("wsum=", w_sum); o.printV("maxw=", max_w); o.printV("minw=", min_w); 
  o.printV("py_adj=", py_adjust); o.printV("lam_scale=", lam_scale); 
  o.printEnd(); 
}

/*-------------------------------------------------------------------*/
void 
AzRgforest::apply(AzTETrainer_TestData *td, 
                  AzDvect *v_test_p, 
                  AzTE_ModelInfo *info, /* may be NULL */
                  AzTreeEnsemble *out_ens) /* may be NULL */
 const 
{
  const AzDataForTrTree *test_data = AzTETrainer::_data(td); 
  int f_num = -1, nz_f_num = -1; 
  AzBmat *b_test_tran = AzTETrainer::_b(td); 
  if (!isOpt) { /* weights have not been corrected */
    AzTimeLog::print("Testing (branch-off for end-of-training optimization)", out); 
    AzBmat temp_b(b_test_tran); 
    temp_apply_copy_to(out_ens, test_data, &temp_b, v_test_p,  
                       &f_num, &nz_f_num); 
  }
  else {
    AzTimeLog::print("Testing ... ", out); 
    opt->apply(test_data, b_test_tran, ens, v_test_p, /* prediction */
               &f_num, &nz_f_num);    /*info */
    if (out_ens != NULL) {
      ens->copy_to(out_ens, s_config.c_str(), signature()); 
    }
  }

  /*---  info  ---*/
  if (info != NULL) {
    info->leaf_num = l_num; 
    info->tree_num = ens->size(); 
    info->f_num = f_num; 
    info->nz_f_num = nz_f_num; 
    info->s_sign.reset(signature()); 
    info->s_config.reset(&s_config); 
  }
}

/*--------------------------------------------------------*/
void AzRgforest::copy_to(AzTreeEnsemble *out_ens) const 
{
  if (isOpt) {
    ens->copy_to(out_ens, s_config.c_str(), signature()); 
  }
  else {
    AzTimeLog::print(" ... branch off for end-of-training optimization ...", out); 
    temp_apply_copy_to(out_ens, NULL, NULL, NULL, NULL, NULL); 
  }
}

/*--------------------------------------------------------*/
int AzRgforest::adjustTestInterval(int lnum_inc_test, int lnum_inc_opt) 
{
  int org_lnum_inc_test = lnum_inc_test; 
  if (s_temp_for_trees.length() > 0 && 
      lnum_inc_test < lnum_inc_opt) {
    AzPrint::writeln(out, "Warning: test interval must not be smaller than optimization interval when the following is specified: ", kw_temp_for_trees); 
    lnum_inc_test = lnum_inc_opt; 
  }
  else if (lnum_inc_test > lnum_inc_opt) {
    lnum_inc_test = (lnum_inc_test+lnum_inc_opt/2)/lnum_inc_opt*lnum_inc_opt; 
  }
  else if (lnum_inc_opt % lnum_inc_test != 0) {
    int div = lnum_inc_opt / lnum_inc_test; 
    for ( ; ; --div) {
      if (lnum_inc_opt % div == 0) {
        lnum_inc_test = lnum_inc_opt / div; 
        break; 
      }
    }
  }
  if (org_lnum_inc_test != lnum_inc_test) {
    AzBytArr s("Changing test interval: "); 
    s.cn(org_lnum_inc_test); s.c("->"); s.cn(lnum_inc_test); 
    AzPrint::writeln(out, s); 
  }
  return lnum_inc_test; 
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
int AzRgforest::resetParam(AzParam &p)
{
  const char *eyec = "AzRgforest::resetParam"; 

  /*---  for storing data indexes in the trees to disk  ---*/
  /*---  this must be called before adjustTestInterval. ---*/
  p.vStr(kw_temp_for_trees, &s_temp_for_trees); 

  /*---  loss function   ---*/
//  p.vLoss(kw_loss, &loss_type); 
  AzBytArr s_loss; 
  p.vStr(kw_loss, &s_loss); 
  if (s_loss.length() > 0) loss_type = AzLoss::lossType(s_loss.c_str()); 
  
  /*---  weight optimization interval  ---*/
  int lnum_inc_opt = lnum_inc_opt_dflt; 
  p.vInt(kw_lnum_inc_opt, &lnum_inc_opt); 
  if (lnum_inc_opt <= 0) {
    throw new AzException(AzInputNotValid, eyec, kw_lnum_inc_opt, 
                          "must be positive"); 
  }
  opt_timer.reset(lnum_inc_opt); 

  /*---  # of trees to search  ---*/
  p.vInt(kw_s_tree_num, &s_tree_num); 
  if (s_tree_num <= 0) {
    throw new AzException(AzInputNotValid, eyec, kw_s_tree_num, 
                          "must be positive"); 
  }

  /*---  when to stop: max #leaf, max #tree  ---*/
  int max_tree_num = -1, max_lnum = max_lnum_dflt; 
  p.vInt(kw_max_tree_num, &max_tree_num); 
  p.vInt(kw_max_lnum, &max_lnum); 
  if (max_tree_num <= 0) {
    if (max_lnum > 0) max_tree_num = MAX(1, max_lnum / 2); 
    else {
      AzBytArr s("Specify "); s.c(kw_max_lnum); s.c(" and/or "); s.c(kw_max_tree_num); 
      throw new AzException(AzInputMissing, eyec, s.c_str()); 
    }
  }
  lmax_timer.reset(max_lnum); 

  /*---  when to test: test interval  ---*/
  int lnum_inc_test = lnum_inc_test_dflt; 
  p.vInt(kw_lnum_inc_test, &lnum_inc_test); 
  if (lnum_inc_test <= 0) {
    throw new AzException(AzInputNotValid, eyec, kw_lnum_inc_test, 
                          "must be positive"); 
  }
  lnum_inc_test = adjustTestInterval(lnum_inc_test, lnum_inc_opt); 

  test_timer.reset(lnum_inc_test); 

  /*---  memory handling  ---*/
  p.vStr(kw_mem_policy, &s_mem_policy); 
  if (s_mem_policy.length() <= 0)                     beTight = false; 
  else if (s_mem_policy.compare(mp_beTight) == 0)     beTight = true; 
  else if (s_mem_policy.compare(mp_not_beTight) == 0) beTight = false; 
  else {
    AzBytArr s(kw_mem_policy); s.c(" should be either ");
    s.c(mp_beTight); s.c(" or "); s.c(mp_not_beTight); 
    throw new AzException(AzInputNotValid, eyec, s.c_str()); 
  }

  p.vFloat(kw_f_ratio, &f_ratio); 
  if (f_ratio > 1) {
    throw new AzException(AzInputNotValid, kw_f_ratio, "must be between 0 and 1."); 
  }
  int random_seed = -1; 
  if (f_ratio > 0 && f_ratio < 1) {
    p.vInt(kw_random_seed, &random_seed); 
    if (random_seed > 0) {
      srand(random_seed); 
    }
  }

  p.swOn(&doPassiveRoot, kw_doPassiveRoot); 

  /*---  for maintenance purposes  ---*/
  p.swOn(&doForceToRefreshAll, kw_doForceToRefreshAll); 
  p.swOn(&beVerbose, kw_forest_beVerbose); /* for compatibility */
  p.swOn(&beVerbose, kw_beVerbose); 
  p.swOn(&doTime, kw_doTime); 

  /*---  display parameters  ---*/
  if (!out.isNull()) {
    AzPrint o(out); 
    o.ppBegin("AzRgforest", "Forest-level"); 
//    o.printLoss(kw_loss, loss_type); 
    o.printV(kw_loss, AzLoss::lossName(loss_type)); 
    o.printV(kw_max_lnum, max_lnum); 
    o.printV(kw_max_tree_num, max_tree_num); 
    o.printV(kw_lnum_inc_opt, lnum_inc_opt); 
    o.printV(kw_lnum_inc_test, lnum_inc_test); 
    o.printV(kw_s_tree_num, s_tree_num); 
    o.printSw(kw_doForceToRefreshAll, doForceToRefreshAll); 
    o.printSw(kw_beVerbose, beVerbose); 
    o.printSw(kw_doTime, doTime); 
    o.printV_if_not_empty(kw_mem_policy, s_mem_policy); 
    o.printV_if_not_empty(kw_temp_for_trees, &s_temp_for_trees); 
    o.printV(kw_f_ratio, f_ratio); 
    o.printV(kw_random_seed, random_seed); 
    o.printSw(kw_doPassiveRoot, doPassiveRoot); 
    o.ppEnd(); 
  }

  if (loss_type == AzLoss_None) {
    throw new AzException(AzInputNotValid, eyec, kw_loss); 
  }

  return max_tree_num; 
}
 
/*------------------------------------------------*/
void AzRgforest::printHelp(AzHelp &h) const
{
  fs->printHelp(h); 

  h.begin(Azforest_config, "AzRgforest", "Forest-wide control"); 
  h.item(kw_loss, help_loss, AzLoss::lossName(loss_type_dflt)); 
  AzDataPool<AzBytArr> pool_desc; 
  AzLoss::help_lines(h.getLevel(), &pool_desc); 
  int ix; 
  for (ix = 0; ix < pool_desc.size(); ++ix) {
    h.writeln_desc(pool_desc.point(ix)->c_str()); 
  }
  h.item(kw_max_lnum, help_max_lnum, max_lnum_dflt); 
  h.item_experimental(kw_max_tree_num, help_max_tree_num, "Don't care"); 
  h.item(kw_lnum_inc_opt, help_lnum_inc_opt, lnum_inc_opt_dflt); 
  h.item(kw_lnum_inc_test, help_lnum_inc_test, lnum_inc_test_dflt); 
  h.item(kw_s_tree_num, help_s_tree_num, s_tree_num_dflt);

  h.item_experimental(kw_temp_for_trees, help_temp_for_trees); 
  h.item_experimental(kw_f_ratio, help_f_ratio); 
  h.item_experimental(kw_doPassiveRoot, help_doPassiveRoot); 
  h.end(); 

  reg_depth->printHelp(h);  
  opt->printHelp(h); 
  ens->printHelp(h); 

  h.begin(Azforest_config, "AzRgforest", "Info display"); 
  h.item(kw_doTime, help_doTime); 
  h.item(kw_beVerbose, help_beVerbose); 
  h.item(kw_mem_policy, help_mem_policy, mp_not_beTight); 
  h.end();
}
