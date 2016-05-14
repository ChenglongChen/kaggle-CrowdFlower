/* * * * *
 *  AzOptOnTree.hpp 
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

#ifndef _AZ_OPT_ON_TREE_HPP_
#define _AZ_OPT_ON_TREE_HPP_

#include "AzUtil.hpp"
#include "AzSmat.hpp"
#include "AzDmat.hpp"
#include "AzLoss.hpp"
#include "AzSvFeatInfo.hpp"
#include "AzRgf_kw.hpp"
#include "AzTrTreeFeat.hpp"
#include "AzTrTreeEnsemble_ReadOnly.hpp"
#include "AzOptimizerT.hpp"
#include "AzRegDepth.hpp"
#include "AzParam.hpp"

class AzRgf_forDelta {
public:
  int changed, truncated; 
  double sum_delta, my_max; 
  AzRgf_forDelta() : changed(0), truncated(0), sum_delta(0), my_max(0) {}

  void check_delta(double *delta, //<! inout 
                   double max_delta); 
  double avg_delta() const; 
}; 

//! coordinate descent for weight optimization. 
/*--------------------------------------------------------*/
class AzOptOnTree : /* implements */ public virtual AzOptimizerT
{
protected: 
  AzDvect v_w; //!< [tree_feature] weights 
  AzDvect v_y; //!< [data_point] target  
  AzDvect v_p;   //!< [data_point] prediction 

  AzDvect v_fixed_dw; //!< [data point] user-defined data point weights

  double var_const, fixed_const; 

  const AzRegDepth *reg_depth; 

  /*-----  parameters  -----*/
  double eta, lambda, sigma, exit_delta, max_delta; 
  AzLossType loss_type; 
  int max_ite_num; 
  bool doRefreshP, doIntercept, doUnregIntercept, doUseAvg; 
  AzOut out, my_dmp_out; 

  /*---  just pointing  ---*/
/*  const AzTrTreeEnsemble_ReadOnly *ens; */
  const AzRgfTreeEnsemble *ens; 
  const AzTrTreeFeat *tree_feat; 

  AzIntArr ia_empty; 

  /*---  default values  ---*/
  static const int max_ite_num_dflt_oth = 10; 
  static const int max_ite_num_dflt_expo = 5; 
  static const AzLossType loss_type_dflt = AzLoss_Square; 
  #define eta_dflt 0.5
  #define exit_delta_dflt -1
  #define max_delta_dflt -1

public: 
  AzOptOnTree() : reg_depth(NULL), 
    var_const(0), fixed_const(0), 
    eta(eta_dflt), lambda(-1), sigma(sigma_dflt), 
    exit_delta(exit_delta_dflt), max_delta(max_delta_dflt), 
    loss_type(loss_type_dflt), max_ite_num(-1),
    doIntercept(false), /* changed on 12/09/2011 */
    doRefreshP(false), doUnregIntercept(false), doUseAvg(false),  
    ens(NULL), tree_feat(NULL)
    {}

  ~AzOptOnTree() {}

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
             const AzDvect *inp_v_p=NULL);

  virtual void printHelp(AzHelp &h) const; 
  virtual void printParam(const AzOut &out, bool beVerbose) const; 

  virtual void optimize(AzRgfTreeEnsemble *ens, /* weights are updated */
                       const AzTrTreeFeat *tree_feat, 
                       int inp_ite_num=-1, 
                       double lam=-1, 
                       double sig=-1) {
    optimize(ens, tree_feat, false, inp_ite_num, lam, sig); 
  }
  virtual void optimize(AzRgfTreeEnsemble *ens, /* weights are updated */
                       const AzTrTreeFeat *tree_feat, 
                       bool doRefreshP, 
                       int inp_ite_num=-1, 
                       double lam=-1, 
                       double sig=-1); 

  inline AzLossType lossType() const {
    return loss_type; 
  }

  /*---  ---*/
  inline const AzDvect *weights() const {
    return &v_w; 
  }
  inline double constant() const {
    return fixed_const + var_const; 
  }
  void dumpWeights(const AzOut &out, 
                int print_max = 50, 
                bool changeLine = true) const; 

  inline void copyPred_to(AzDvect *out_v_p) const {
    out_v_p->reform(v_p.rowNum()); 
    out_v_p->set(&v_p); 
  }

  /*---  ---*/
  void copy_from(const AzOptOnTree *inp); 
  void reset(const AzOptOnTree *inp) {
    copy_from(inp); 
  }

  void resetPred(const AzBmat *m_tran, 
                 AzDvect *v_p) /* output */
                 const; 

  /*---*/
  void resetDw(const AzDvect *inp_v_dw) {
    if (!AzDvect::isNull(inp_v_dw)) {
      if (inp_v_dw->rowNum() != v_y.rowNum()) {
        throw new AzException("AzOptOnTree::resetDw", "dimensionality conflict"); 
      }
      v_fixed_dw.set(inp_v_dw); 
    }
    else {
      v_fixed_dw.destroy(); 
    }
  }

protected: 
  void _reset() {
    v_w.reset(); 
    v_p.reset(); 
    v_y.reset(); 
    v_fixed_dw.reset(); 
    var_const = fixed_const = 0; 
  }
  void synchronize(); 

  void iterate(int inp_ite_num, 
               double lam, 
               double sig); 

  virtual double update(double nlam, double nsig); 
  virtual void update_with_features(double nlam, double nsig, double py_avg, 
                            AzRgf_forDelta *for_delta); 
  virtual void _update_with_features(double nlam, double nsig, double py_avg, 
                            AzRgf_forDelta *for_delta);
  virtual void _update_with_features_TempFile(double nlam, double nsig, double py_avg, 
                            AzRgf_forDelta *for_delta);
  void update_intercept(double nlam, double nsig, double py_avg, 
                        AzRgf_forDelta *for_delta); /* updated */

  virtual double getDelta(const int *dxs, int dxs_num, double w,
                  double nlam, double nsig, double py_avg, 
                  /*---  inout  ---*/
                  AzRgf_forDelta *for_delta) 
                  const; 

  virtual void checkParam() const; 

  virtual void monitorLoss(int ite, 
                   double delta, 
                   const AzOut &out) const; 
  double getDelta(double w, 
                  double dL, 
                  double ddL); 
  inline const AzTrTreeNode *node(int fx) const
  {
    const char *eyec = "AzOptOnTree::node"; 
    const AzTrTreeFeatInfo *fp = tree_feat->featInfo(fx); 
    if (fp->tx < 0) throw new AzException(eyec, "negative tx"); 
    if (fp->nx < 0) throw new AzException(eyec, "negative nx"); 
    return ens->tree(fp->tx)->node(fp->nx); 
  }

  inline const int *data_points(int fx, int *num) const
  {
    *num = node(fx)->dxs_num; 
    return node(fx)->data_indexes(); 
  }

  virtual void refreshPred(); 
  virtual void _refreshPred(); 
  virtual void _refreshPred_TempFile(); 
  inline static void updatePred(const int *dxs, int dxs_num, double delta, 
                                AzDvect *out_v_p) {
    out_v_p->add(delta, dxs, dxs_num);   
  }
  virtual void resetParam(AzParam &param); 

  void updateTreeWeights(AzRgfTreeEnsemble *ens) const; 

  virtual void _warmup(const AzTrTreeEnsemble_ReadOnly *ens, 
                    const AzTrTreeFeat *tree_feat, 
                    const AzDvect *inp_v_p); 
}; 

#endif 
