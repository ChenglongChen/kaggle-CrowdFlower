/* * * * *
 *  AzOptOnTree_TreeReg.cpp 
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

#include "AzOptOnTree_TreeReg.hpp"
#include "AzPrint.hpp"

/*--------------------------------------------------------*/
void
AzOptOnTree_TreeReg::optimize(AzRgfTreeEnsemble *inp_rgf_ens, 
                      const AzTrTreeFeat *inp_tree_feat, 
                      int ite_num, 
                      double lam, 
                      double sig)
{
  ens = inp_rgf_ens;
  tree_feat = inp_tree_feat; 
  rgf_ens = inp_rgf_ens;  

  synchronize(); 
  updateTreeWeights(rgf_ens); 

  int tree_num = ens->size(); 
  if (reg_arr->size() < tree_num) {
    throw new AzException("AzOptOnTree_TreeReg::optimize", 
                          "max #tree has changed??"); 
  }
  int tx; 
  for (tx = 0; tx < tree_num; ++tx) {
    AzReg_TreeReg *reg = reg_arr->reg(tx);  
    reg->reset(ens->tree(tx), reg_depth); 
  }
  iterate(ite_num, lam, sig); 

  ens = NULL; 
  tree_feat = NULL; 
  rgf_ens = NULL; 
}

/*--------------------------------------------------------*/
void AzOptOnTree_TreeReg::update_with_features(
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_delta) /* updated */
{
  int tree_num = ens->size();
  int tx; 
  for (tx = 0; tx < tree_num; ++tx) {
    ens->tree_u(tx)->restoreDataIndexes(); 
    AzReg_TreeReg *reg = reg_arr->reg(tx); 
    reg->clearFocusNode(); 

    AzIIarr iia_nx_fx; 
    tree_feat->featIds(tx, &iia_nx_fx); 
    int num = iia_nx_fx.size(); 
    AzIIFarr iifa_nx_fx_delta; 
    int ix; 
    for (ix = 0; ix < num; ++ix) {
      int nx, fx; 
      iia_nx_fx.get(ix, &nx, &fx); 

      double delta = bestDelta(nx, fx, reg, nlam, nsig, py_avg, for_delta); 
      update_weight(nx, fx, delta, reg);
    }
    ens->tree_u(tx)->releaseDataIndexes(); 
  }
}                                         
/*--------------------------------------------------------*/
void AzOptOnTree_TreeReg::update_weight(int nx, 
                                   int fx, 
                                   double delta,
                                   AzReg_TreeReg *reg)
{
  double new_w = v_w.get(fx) + delta; 
  v_w.set(fx, new_w); 

  int dxs_num; 
  const int *dxs = data_points(fx, &dxs_num); 
  updatePred(dxs, dxs_num, delta, &v_p); 

  /*---  update the weight in the ensemble  ---*/ 
  const AzTrTreeFeatInfo *fp = tree_feat->featInfo(fx); 
  rgf_ens->tree_u(fp->tx)->setWeight(fp->nx, new_w); 
  reg->changeWeight(nx, delta); 
}

/*--------------------------------------------------------*/
double AzOptOnTree_TreeReg::bestDelta(
                      int nx, 
                      int fx, 
                      AzReg_TreeReg *reg, 
                      double nlam, 
                      double nsig, 
                      double py_avg, 
                      AzRgf_forDelta *for_delta) /* updated */
const
{
  const char *eyec = "AzOptOnTree_TI::bestDelta"; 

  double w = v_w.get(fx); 
  int dxs_num; 
  const int *dxs = data_points(fx, &dxs_num); 
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

  double dR, ddR; 
  reg->penalty_deriv(nx, &dR, &ddR); 

  double dd = ddL + nlam*ddR; 
  if (dd == 0) dd = 1; 
  double delta = (nega_dL-nlam*dR)*eta/dd; 
  for_delta->check_delta(&delta, max_delta); 

  return delta; 
}
