/* * * * *
 *  AzOptOnTree.cpp 
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

#include "AzOptOnTree.hpp"
#include "AzTaskTools.hpp"
#include "AzHelp.hpp"

/*--------------------------------------------------------*/
void AzOptOnTree::reset(AzLossType l_type, 
                        const AzDvect *inp_v_y, 
                        const AzDvect *inp_v_fixed_dw, /* user-assigned data point weights */
                        const AzRegDepth *inp_reg_depth, 
                        AzParam &param, 
                        bool beVerbose, 
                        const AzOut out_req, 
                        /*---  for warm start  ---*/
                        const AzTrTreeEnsemble_ReadOnly *inp_ens, 
                        const AzTrTreeFeat *inp_tree_feat, 
                        const AzDvect *inp_v_p)

{
  _reset(); 

  reg_depth = inp_reg_depth; 
  out = out_req; 
  my_dmp_out = dmp_out; 

  v_y.set(inp_v_y); 
  if (!AzDvect::isNull(inp_v_fixed_dw)) {
    v_fixed_dw.set(inp_v_fixed_dw); 
  }
  v_p.reform(v_y.rowNum()); 
  loss_type = l_type; 

  resetParam(param); 

  if (max_delta < 0 && 
      AzLoss::isExpoFamily(loss_type)) {
    max_delta = 1; 
  }

  printParam(out, beVerbose); 
  checkParam(); 

  if (!beVerbose) {
    out.deactivate();  
    my_dmp_out.deactivate(); 
  }

  var_const = 0; 
  fixed_const = 0; 
  if (doUseAvg) {
    fixed_const = v_y.sum() / (double)v_y.rowNum(); 
    v_p.set(fixed_const); 
  }

  if (inp_ens != NULL) {
    _warmup(inp_ens, inp_tree_feat, inp_v_p); 
  }
}

/*--------------------------------------------------------*/
void 
AzOptOnTree::_warmup(const AzTrTreeEnsemble_ReadOnly *inp_ens, 
                    const AzTrTreeFeat *inp_tree_feat, 
                    const AzDvect *inp_v_p)
{
  v_w.reform(inp_tree_feat->featNum()); 
  var_const = inp_ens->constant() - fixed_const; 

  int fx; 
  int f_num = inp_tree_feat->featNum(); 
  for (fx = 0; fx < f_num; ++fx) {
    const AzTrTreeFeatInfo *fp = inp_tree_feat->featInfo(fx); 
    if (fp->isRemoved) continue; 
    double w = inp_ens->tree(fp->tx)->node(fp->nx)->weight; 
    v_w.set(fx, w); 
  }

  v_p.set(inp_v_p); 
}

/*--------------------------------------------------------*/
void AzOptOnTree::copy_from(const AzOptOnTree *inp)
{
  v_w.set(&inp->v_w); 
  v_y.set(&inp->v_y); 
  v_fixed_dw.set(&inp->v_fixed_dw); 
  v_p.set(&inp->v_p); 

  var_const = inp->var_const; 
  fixed_const = inp->fixed_const; 

  /*-----  parameters  -----*/
  eta = inp->eta; 
  lambda = inp->lambda; 
  sigma = inp->sigma; 
  exit_delta = inp->exit_delta; 
  max_delta = inp->max_delta; 
  reg_depth = inp->reg_depth; 
  loss_type = inp->loss_type; 
  max_ite_num = inp->max_ite_num; 

  doRefreshP = inp->doRefreshP; 
  doIntercept = inp->doIntercept; 
  doUnregIntercept = inp->doUnregIntercept; 
  doUseAvg = inp->doUseAvg; 

  ens = NULL; 
  tree_feat = NULL; 

  out = inp->out; 
  my_dmp_out = inp->my_dmp_out; 
}

/*--------------------------------------------------------*/
void AzOptOnTree::synchronize()
{
  checkParam();

  int f_num = tree_feat->featNum(); 
  int old_f_num = v_w.rowNum(); 
  v_w.resize(f_num); 

  bool isThereChange = false; 
  int fx; 
  for (fx = 0; fx < old_f_num; ++fx) {
    if (tree_feat->featInfo(fx)->isRemoved && v_w.get(fx) != 0) {
      isThereChange = true; 
      v_w.set(fx, 0); 
    }
  }
  if (isThereChange || doRefreshP) {
    refreshPred(); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::iterate(int inp_ite_num, 
                             double lam, 
                             double sig)
{
  int ite_num = max_ite_num; 
  if (inp_ite_num >= 0) {
    ite_num = inp_ite_num; 
  }
  if (ite_num > 0 && !out.isNull()) {
    AzPrint o(out); 
    o.reset_options(); 
    o.printBegin("AzOptOnTree::update", ","); 
    o.printV("lam=", (lam>=0)?lam:lambda); 
    o.printV_posiOnly("sig=", (sig>=0)?sig:sigma); 
//    o.printLoss("", loss_type); 
    o.printV("", AzLoss::lossName(loss_type)); 
    o.printEnd(); 
  }
  if (ite_num <= 0) {
    return; 
  }

  double nn; 
  if (AzDvect::isNull(&v_fixed_dw)) nn = v_y.rowNum(); 
  else                              nn = v_fixed_dw.sum(); 
  double nlam = lambda * nn; 
  if (lam >= 0) {
    nlam = lam * nn; 
  }
  double nsig = sigma * nn; 
  if (sig >= 0) {
    nsig = sig * nn; 
  }

  bool doExit = false; 
  int ite_chk = MIN(5, ite_num); 
  int ite; 
  for (ite = 0; ite < ite_num; ++ite) {
    double delta = update(nlam, nsig); 
    if (exit_delta > 0 && 
        delta < exit_delta) {
      doExit = true; 
    }
    if (!out.isNull() && (ite+1 == ite_chk || doExit)) {
      monitorLoss(ite, delta, out); 
      if (ite_chk < 10)       ite_chk += 5; 
      else                    ite_chk += 10;  
      ite_chk = MIN(ite_chk, ite_num); 
    }
    if (doExit) {
      AzTimeLog::print("Reached exiting criteria", out); 
      break; 
    }
  }
  dumpWeights(my_dmp_out); 
}

/*--------------------------------------------------------*/
void AzOptOnTree::monitorLoss(int ite, 
                                  double delta, 
                                  const AzOut &out) const 
{
  if (out.isNull()) return; 

  const double *y = v_y.point(); 
  const double *fixed_dw = NULL; 
  double nn = v_y.rowNum(); 
  if (!AzDvect::isNull(&v_fixed_dw)) {
    fixed_dw = v_fixed_dw.point(); 
    nn = v_fixed_dw.sum(); 
  }
  int data_num = v_p.rowNum(); 
  const double *p = v_p.point(); 
  double uloss_sum=0; 
  int t=0,g=0,ok=0; 
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    if (p[dx] > 0) ++t; 
    if (y[dx] > 0) ++g; 
    if (y[dx] > 0 && p[dx] > 0) ++ok; 
    double uloss = AzLoss::getLoss(loss_type, p[dx], y[dx]); 
    if (fixed_dw != NULL) uloss *= fixed_dw[dx]; 
    uloss_sum += uloss; 
  }

  double uloss_avg = uloss_sum / nn; 
  double n1 = v_w.absSum(); 
  double n2 = v_w.selfInnerProduct(); 

  /* ite 1,delta,uloss=..,n1,n2,pr,p,r\n */
  AzPrint o(out); 
  o.printBegin("", ",", "="); 
  o.print("ite", ite+1, 2); 
  bool doSci = true; 
  o.print(delta, 4, doSci); 
  o.print("uloss", uloss_avg, 4, doSci); 
  o.print(n1, 3, doSci); o.print(n2, 3, doSci); 
  if (g > 0 && g < data_num) {
    AzTaskTools::printPR(o, ok, t, g); 
  }
  o.printEnd(); 
  o.flush(); 
}

/*--------------------------------------------------------*/
double AzOptOnTree::update(double inp_nlam, 
                               double nsig)
{
  double nlam = inp_nlam; 

  /*---  for numerical stability  ---*/
  double py_avg = 0; 
  if (loss_type == AzLoss_Expo) {
    py_avg = AzLoss::py_avg(&v_p, &v_y); 
    double nlam_scale = AzLoss::lamScale(py_avg); 
    if (nlam > 0) {
      nlam *= nlam_scale; 
    }
    if (nsig > 0) {
      nsig *= nlam_scale; 
    }
  }

  AzRgf_forDelta for_delta; 

  update_with_features(nlam, nsig, py_avg, &for_delta); 
  update_intercept(nlam, nsig, py_avg, &for_delta); 

  if (for_delta.truncated > 0 && !out.isNull()) {
    AzPrint o(out); 
    o.printBegin("", ""); 
    o.print("AzOptOnTree::update,truncated=", for_delta.truncated); 
    o.printEnd(); 
  }

  double abs_delta_avg = for_delta.avg_delta(); 
  return abs_delta_avg; 
}

/*--------------------------------------------------------*/
void AzOptOnTree::_update_with_features(
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_del) /* updated */
{
  int fx; 
  int f_num = tree_feat->featNum(); 
  for (fx = 0; fx < f_num; ++fx) {
    if (tree_feat->featInfo(fx)->isRemoved) continue; 

    double w = v_w.get(fx); 
    int dxs_num; 
    const int *dxs = data_points(fx, &dxs_num); 
    double my_nlam = reg_depth->apply(nlam, node(fx)->depth); 
    double my_nsig = reg_depth->apply(nsig, node(fx)->depth); 
    double delta = getDelta(dxs, dxs_num, w, my_nlam, my_nsig, py_avg, for_del); 
    v_w.set(fx, w+delta); 
    updatePred(dxs, dxs_num, delta, &v_p); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::_update_with_features_TempFile(
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_del) /* updated */
{
  int tree_num = ens->size();
  int tx; 
  for (tx = 0; tx < tree_num; ++tx) {
    ens->tree_u(tx)->restoreDataIndexes(); 
    AzIIarr iia_nx_fx; 
    tree_feat->featIds(tx, &iia_nx_fx); 
    int num = iia_nx_fx.size(); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int nx, fx; 
      iia_nx_fx.get(ix, &nx, &fx); 
      if (tree_feat->featInfo(fx)->isRemoved) continue; /* shouldn't happen though */

      double w = v_w.get(fx); 
      int dxs_num; 
      const int *dxs = data_points(fx, &dxs_num); 
      double my_nlam = reg_depth->apply(nlam, node(fx)->depth); 
      double my_nsig = reg_depth->apply(nsig, node(fx)->depth); 
      double delta = getDelta(dxs, dxs_num, w, my_nlam, my_nsig, py_avg, for_del); 
      v_w.set(fx, w+delta); 
      updatePred(dxs, dxs_num, delta, &v_p); 
    }
    ens->tree_u(tx)->releaseDataIndexes(); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::update_with_features(
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_del) /* updated */
{
  if (ens->usingTempFile()) {
    _update_with_features_TempFile(nlam, nsig, py_avg, for_del); 
  }
  else {
    _update_with_features(nlam, nsig, py_avg, for_del); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::update_intercept(
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_delta) /* updated */
{
  /*---  intercept  ---*/
  if (doIntercept || doUnregIntercept) {
    double my_nlam = nlam, my_nsig = nsig; 
    if (doUnregIntercept) {
      my_nlam = my_nsig = 0; 
    }
    AzIntArr ia_all_dx; 
    ia_all_dx.range(0, v_p.rowNum());
    double delta = getDelta(ia_all_dx.point(), ia_all_dx.size(), 
                            var_const, my_nlam, my_nsig, 
                            py_avg, for_delta); 
    var_const += delta; 
    updatePred(ia_all_dx.point(), ia_all_dx.size(), delta, &v_p); 
  }
}

/*--------------------------------------------------------*/
double AzOptOnTree::getDelta(const int *dxs, 
                             int dxs_num, 
                                 double w,
                                 double nlam, 
                                 double nsig, 
                                 double py_avg, 
                                 /*---  inout  ---*/
                                 AzRgf_forDelta *for_del) /* updated */
const 
{
  const char *eyec = "AzOptOnTree::getDelta"; 
  if (dxs == NULL) return 0; 
  if (dxs_num <= 0) {
    throw new AzException(eyec, "no data indexes"); 
  }

  const double *fixed_dw = NULL; 
  if (!AzDvect::isNull(&v_fixed_dw)) fixed_dw = v_fixed_dw.point(); 

  const double *p = v_p.point(); 
  const double *y = v_y.point(); 

  double nega_dL = 0, ddL= 0; 
  if (fixed_dw == NULL) {
    AzLoss::sum_deriv(loss_type, dxs, dxs_num, p, y, py_avg, 
                      nega_dL, ddL); 
  }
  else {
    AzLoss::sum_deriv_weighted(loss_type, dxs, dxs_num, p, y, fixed_dw, py_avg, 
                      nega_dL, ddL); 
  }

  double ddL_nlam = ddL + nlam; 
  if (ddL_nlam == 0) ddL_nlam = 1;  /* this shouldn't happen, though */
  double delta = (nega_dL-nlam*w)*eta/ddL_nlam; 
  if (nsig > 0) {
    double del1; 
    if (w+delta>0) del1 = delta - nsig*eta/ddL_nlam; 
    else           del1 = delta + nsig*eta/ddL_nlam; 
    if ((w+delta)*(w+del1) <= 0) delta = -w; 
    else                         delta = del1; 
  }

  for_del->check_delta(&delta, max_delta); 
  return delta; 
}       

/*--------------------------------------------------------*/
void AzOptOnTree::checkParam() const
{
  const char *eyec = "AzOptOnTree::checkParam"; 
  if (lambda < 0) {
    throw new AzException(AzInputNotValid, eyec, 
                          kw_lambda, "must be non-negative"); 
  }
  if (sigma < 0) {
    throw new AzException(AzInputNotValid, eyec, 
                          kw_sigma, "must be non-negative"); 
  }
  if (eta <= 0) {
    throw new AzException(AzInputNotValid, eyec, kw_eta, "must be positive"); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::dumpWeights(const AzOut &out,                        
                        int print_max, 
                        bool changeLine) const 
{
  if (out.isNull()) return; 
  int nz = v_w.nonZeroRowNum(); 

  AzPrint o(out); 
  o.printBegin("", ",", "="); 
  o.print("#non_zero_weight", nz); 
  o.print("var_const", var_const, 5); 
  o.print("fixed_const", fixed_const, 5); 
  o.printEnd(); 
  o.writeln(""); 
  const char *name = "OUT"; 
  AzTaskTools::dumpWeights(out, &v_w, name, tree_feat, print_max, changeLine); 
}

/*--------------------------------------------------------*/
void 
AzOptOnTree::optimize(AzRgfTreeEnsemble *rgf_ens, 
                      const AzTrTreeFeat *inp_tree_feat, 
                      bool doRefreshP, 
                      int ite_num, 
                      double lam, 
                      double sig)
{
  ens = rgf_ens; 
  tree_feat = inp_tree_feat; 
  synchronize(); 
  if (doRefreshP) {
    refreshPred(); 
  }
  iterate(ite_num, lam, sig); 
  updateTreeWeights(rgf_ens); 
  ens = NULL; 
  tree_feat = NULL; 
}

/*------------------------------------------------------------------*/
void AzOptOnTree::updateTreeWeights(AzRgfTreeEnsemble *ens) const
{
  int dtree_num = ens->size(); 
  int tx; 
  for (tx = 0; tx < dtree_num; ++tx) {
    ens->tree_u(tx)->resetWeights(); 
  }

  const double *weight = weights()->point(); 
  double const_val = constant(); 
  ens->set_constant(const_val); 

  int num = tree_feat->featNum(); 
  int fx; 
  for (fx = 0; fx < num; ++fx) {
    if (weight[fx] != 0) {
      const AzTrTreeFeatInfo *fp = tree_feat->featInfo(fx); 
      ens->tree_u(fp->tx)->setWeight(fp->nx, weight[fx]); 
    }
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::_refreshPred()
{
  if (v_w.rowNum() == 0 && v_p.rowNum() == 0) return; 

  v_p.zeroOut(); 
  v_p.set(var_const+fixed_const);  

  const double *w = v_w.point(); 
  int f_num = v_w.rowNum(); 
  int fx; 
  for (fx = 0; fx < f_num; ++fx) {
    if (tree_feat->featInfo(fx)->isRemoved) continue; 
    int dxs_num; 
    const int *dxs = data_points(fx, &dxs_num); 
    updatePred(dxs, dxs_num, w[fx], &v_p); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::_refreshPred_TempFile()
{
  if (v_w.rowNum() == 0 && v_p.rowNum() == 0) return; 

  v_p.zeroOut(); 
  v_p.set(var_const+fixed_const);  

  const double *w = v_w.point(); 

  int tree_num = ens->size(); 
  int tx; 
  for (tx = 0; tx < tree_num; ++tx) {
    ens->tree_u(tx)->restoreDataIndexes(); 
    AzIIarr iia_nx_fx; 
    tree_feat->featIds(tx, &iia_nx_fx); 
    int num = iia_nx_fx.size(); 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int nx, fx; 
      iia_nx_fx.get(ix, &nx, &fx); 

      if (tree_feat->featInfo(fx)->isRemoved) continue; 
      int dxs_num; 
      const int *dxs = data_points(fx, &dxs_num); 
      updatePred(dxs, dxs_num, w[fx], &v_p); 
    }
    ens->tree_u(tx)->releaseDataIndexes(); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::refreshPred()
{
  if (ens->usingTempFile()) {
    _refreshPred_TempFile(); 
  }
  else {
    _refreshPred(); 
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::resetPred(const AzBmat *b_tran, 
                                AzDvect *out_v_p) /* output */
const
{
  int data_num = b_tran->rowNum(); 
  out_v_p->reform(data_num); 
  out_v_p->set(var_const+fixed_const); 
  AzCursor cursor; 
  for ( ; ; ) {
    double val; 
    int fx = v_w.next(cursor, val); 
    if (fx < 0) break; 
    const AzIntArr *ia_dx = b_tran->on_rows(fx); 
    updatePred(ia_dx->point(), ia_dx->size(), val, out_v_p); 
  }  
}
 
/*------------------------------------------------------------------*/
void AzRgf_forDelta::check_delta(double *delta, /* inout */
                                double max_delta) 
{
  if (max_delta > 0) {
    double org_delta = *delta; 
    *delta = MAX(-max_delta, MIN(max_delta, *delta)); 
    if (*delta != org_delta) {
      ++truncated; 
    }
  }
  if (*delta != 0) {
    double abs_del = fabs(*delta); 
    my_max = MAX(my_max, abs_del); 
    sum_delta += abs_del; 
    ++changed; 
  }
}

/*------------------------------------------------------------------*/
double AzRgf_forDelta::avg_delta() const 
{
  double avg = sum_delta; 
  if (changed != 0) avg /= (double)changed; 
  return avg; 
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzOptOnTree::printHelp(AzHelp &h) const
{
  h.begin(Azopt_config, "AzOptOnTree"); 
  h.item_required_lvl(kw_lambda, help_lambda, 1); 
  h.item_experimental(kw_sigma, help_sigma, sigma_dflt); 
  h.item(kw_doUseAvg, help_doUseAvg); 
  AzBytArr s_dflt; 
  s_dflt.cn(max_ite_num_dflt_oth);  s_dflt.c(help_oth_loss); s_dflt.c("; "); 
  s_dflt.cn(max_ite_num_dflt_expo); s_dflt.c(help_expo_loss); 
  h.item(kw_max_ite_num, help_max_ite_num, s_dflt.c_str()); 
  h.item_experimental(kw_doIntercept, help_doIntercept); 
  h.item(kw_eta, help_eta, eta_dflt); 
  h.item_experimental(kw_exit_delta, help_exit_delta, exit_delta_dflt); 
  h.end(); 
}

/*------------------------------------------------------------------*/
void AzOptOnTree::resetParam(AzParam &p)
{
  p.vFloat(kw_lambda, &lambda); 
  p.vFloat(kw_sigma, &sigma); 
  p.vInt(kw_max_ite_num, &max_ite_num); 
  p.vFloat(kw_eta, &eta); 
  p.vFloat(kw_exit_delta, &exit_delta); 
  p.vFloat(kw_max_delta, &max_delta); 
  p.swOn(&doUseAvg, kw_doUseAvg); 
  p.swOff(&doIntercept, kw_not_doIntercept); /* useless but keep this for compatibility */
  p.swOn(&doIntercept, kw_doIntercept); 

  if (max_ite_num <= 0) {
    max_ite_num = max_ite_num_dflt_oth; 
    if (AzLoss::isExpoFamily(loss_type)) {
      max_ite_num = max_ite_num_dflt_expo; 
    }
  }
}

/*--------------------------------------------------------*/
void AzOptOnTree::printParam(const AzOut &out, 
                             bool beVerbose) const 
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  o.reset_options(); 
  o.set_precision(5); 
  o.ppBegin("AzOptOnTree", "Optimization"); 
  o.printV("loss=", AzLoss::lossName(loss_type)); 
  o.printV(kw_max_ite_num, max_ite_num); 
  o.printV(kw_lambda, lambda); 
  o.printV_posiOnly(kw_sigma, sigma); 
  o.printV(kw_eta, eta); 
  o.printV(kw_exit_delta, exit_delta); 
  o.printV(kw_max_delta, max_delta); 

  o.printSw(kw_doUseAvg, doUseAvg); 
  o.printSw(kw_doIntercept, doIntercept); 

  o.printSw(kw_opt_beVerbose, beVerbose); 

  /*---  these are fixed; displaying for maintenance purpose only  ---*/
  o.printSw("doRefershP", doRefreshP); 
  o.printSw("doUnregIntercept", doUnregIntercept);  /* unregularized intercept */

  o.ppEnd(); 
}

#if 0 
/*--------------------------------------------------------*/

#endif 