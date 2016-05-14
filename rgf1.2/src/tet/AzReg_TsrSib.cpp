/* * * * *
 *  AzReg_TsrSib.cpp 
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

#include "AzReg_TsrSib.hpp"
#include "AzPrint.hpp"

static bool doShortCut = true; 

/*
 * To evaluate node split: 
 *   Assume two new leaf nodes under the node on focus (the node to be split)
 *   with the weights same as the node on focus, so that predictions stay the same. 
 */

/*--------------------------------------------------------*/
void AzReg_TsrSib::changeWeight(int nx, double w_diff)
{
  if (w_diff != 0) {
    v_v.add(av_dv.point(nx), w_diff); 
  }
}

/*--------------------------------------------------------*/
double AzReg_TsrSib::penalty_diff(const double leaf_w_delta[2])
const
{
  checkLeaf("penalty_diff"); 

  double diff = 0; 

  /*---  on the existing nodes  ---*/
  double delsum = leaf_w_delta[0] + leaf_w_delta[1]; 
  diff += vdv_sum * delsum; 
  diff += (dv2_sum * (delsum*delsum) * 0.5); 

  /*---  on the new leaf nodes  ---*/
  double leaf_v = (leaf_w_delta[0] - leaf_w_delta[1])/2; 
#if 0 
  diff += newleaf_dep_factor*leaf_v*leaf_v/2*2; /* "*2" for two leaves */
#else
  diff += newleaf_dep_factor*leaf_v*leaf_v; /* "/2" and "*2" (for two leaves) cancel out */
#endif 

  return diff; 
}

/*--------------------------------------------------------*/
void AzReg_TsrSib::penalty_deriv(int nx, double *dr, 
                              double *ddr) 
{
  focus_nx = nx; 
  update(); 
  penalty_deriv(dr, ddr); 
}

/*--------------------------------------------------------*/
/**
  *  dr=sum_j v_j dv_j; ddr=sum_j dv_j^2
  *  v_j = bar_j - bar_{p(j)} where p(j) is j's parent. 
 **/
void AzReg_TsrSib::penalty_deriv(double *out_dr, 
                             double *out_ddr) const 
{
  if (focus_nx < 0) {
    throw new AzException("AzReg_TsrSib::penalty_deriv", "no focus node"); 
  }
  *out_dr = dr; 
  *out_ddr = ddr; 
}

/*--------------------------------------------------------*/
/* for optimization */
void AzReg_TsrSib::reset(const AzTrTree_ReadOnly *inp_tree, 
                         const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  forNewLeaf = false; 
  focus_nx = -1; 
  reg_depth = inp_reg_depth; 

  if (tree == NULL) {
    throw new AzException("AzReg_TsrSib::reset", "null tree"); 
  }

  int node_num = tree->nodeNum(); 
  av_dv.reset(node_num);  
  v_v.reform(node_num);  
 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) continue; 

    deriv_v(tree, nx, false, av_dv.point_u(nx), &v_v); 
  }
  reset_values(); 
}

/*--------------------------------------------------------*/
/* for findSplit */
void AzReg_TsrSib::reset_forNewLeaf(const AzTrTree_ReadOnly *inp_tree, 
                            const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  forNewLeaf = true; 
  focus_nx = -1; 
  reg_depth = inp_reg_depth; 

  int node_num = tree->nodeNum(); 

  /*---  set v  ---*/
  v_v.reform(node_num); 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) continue; 

    deriv_v(tree, nx, false, NULL, &v_v); 
  }
  reset_values(); 
}

/*--------------------------------------------------------*/
/* for findSplit */
void AzReg_TsrSib::reset_forNewLeaf(int inp_focus_nx, 
                            const AzTrTree_ReadOnly *inp_tree, 
                            const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  forNewLeaf = true; 
  focus_nx = inp_focus_nx; 
  reg_depth = inp_reg_depth; 

  int node_num = tree->nodeNum(); 

  /*---  set derivatives w.r.t. the weight of the new leaf  ---*/
  av_dv.reset(node_num); 
  AzSvect *v_dv = av_dv.point_u(focus_nx); 
  deriv_v(tree, focus_nx, forNewLeaf, v_dv, NULL); 
  if (v_v.rowNum() != node_num) {
    throw new AzException("AzReg_TsrSib::sib_reset_forNewLeaf", 
                          "v_v is not initialized"); 
  }
  update(); 
}

/*-------------------------------------------------------------*/
/* derivative of beta w.r.t. the leaf weight at node[focus_nx] */ 
void AzReg_TsrSib::deriv_v(const AzTrTree_ReadOnly *tree, 
                           int leaf_nx, 
                           bool forNewLeaf, 
                           /* output */
                           AzSvect *v_dv, 
                           /* inout */
                           AzDvect *v_v)
const
{
  if (v_dv != NULL) {
    v_dv->reform(tree->nodeNum()); 
  }

  double leaf_w = tree->node(leaf_nx)->weight; 

  double pow2 = 0.5;  /* paper: 2^{d_w - d_{u_k} - 1} */
  if (forNewLeaf) {
    pow2 /= 2; 
  }

  double *v_arr = NULL; 
  if (v_v != NULL) v_arr = v_v->point_u(); 

  int nx = leaf_nx; 
  for ( ; ; ) {
    if (nx < 0) break; 

    const AzTrTreeNode *np = tree->node(nx); 
    int px = np->parent_nx; 
    
    double deriv = pow2; 
    if (px < 0) { /* root */
      deriv = pow2*2; 
    }
    if (v_dv != NULL) v_dv->set(nx, deriv); 
    if (v_arr != NULL) v_arr[nx] += deriv*leaf_w; 

    if (px >= 0) {
      /*---  sibling  ---*/
      int sib_nx = tree->node(px)->gt_nx; 
      if (sib_nx == nx) sib_nx = tree->node(px)->le_nx; 
      if (v_dv != NULL) v_dv->set(sib_nx, -deriv); 
      if (v_arr != NULL) v_arr[sib_nx] -= deriv*leaf_w; 
    }

    nx = px; 
    pow2 /= 2; 
  }
}

/*--------------------------------------------------------*/
/**
  *  dr=sum_j v_j dv_j; ddr=sum_j dv_j^2
  *  v_j = bar_j - bar_{p(j)} where p(j) is j's parent. 
  * 
  *  output: vdv_sum, dv2_sum, dr, ddr, and newleaf_dep_factor
 **/
void AzReg_TsrSib::update() 
{
  /* vdv_sum, dv2_sum: don't include new leaf nodes */
  /* dr, ddr:           include new leaf nodes */

  if (focus_nx < 0) {
    throw new AzException("AzReg_TsrSib::update", "no focus node"); 
  }

  dr = ddr = 0; 

  if (forNewLeaf) {
    checkLeaf("update"); 

    /*---  new leaf nodes: v=0, dv=0.5,-0.5  ---*/
    double dv = 0.5; 
    int depth = get_newleaf_depth(); 
    newleaf_dep_factor = reg_depth->apply(1, depth); 
    ddr += dv*dv*newleaf_dep_factor*2; /* *2 for two leaves */
  }
 
  const AzSvect *v_dv = av_dv.point(focus_nx); 
  const double *v_arr = v_v.point(); 

  vdv_sum = dv2_sum = 0; 

  if (doShortCut) {
    /* 
     * Use the fact that v[s]+v[t] = 0, dv[s]+dv[t]=0 if s and t are siblings, 
     * dv only depends on the distance from the leaf, and that 
     * dv<>0 only for the ancestors and ancestors' siblings of the base leaf.  
     */
    double dv = 0.5; 
    if (forNewLeaf) {
      dv = 0.25; 
    }

    int nx = focus_nx; 
    for ( ; ; ) {
      const AzTrTreeNode *np = tree->node(nx); 
      int px = np->parent_nx; 
      int depth = np->depth; 
      double v = v_arr[nx]; 

      if (depth == 0) { /* root */
        dv *= 2; 
        double dv_dep_factor = dv*reg_depth->apply(1, depth);   
        vdv_sum += v*dv_dep_factor; 
        dv2_sum += dv*dv_dep_factor; 
        break; 
      }
      else {
        double dv_dep_factor = dv*reg_depth->apply(1, depth); 
        vdv_sum += 2*v*dv_dep_factor;   /* *2 for the sibling */
        dv2_sum += 2*dv*dv_dep_factor;  /* *2 for the sibling */
      }

      nx = px; 
      dv /= 2; 
    }
  }
  else {
    AzCursor cur; 
    for ( ; ; ) {
      double dv; 
      int nx = v_dv->next(cur, dv); 
      if (nx < 0) break; 

      double v = v_arr[nx]; 
      int depth = tree->node(nx)->depth; 
      double dv_dep_factor = dv*reg_depth->apply(1, depth); 

      vdv_sum += v * dv_dep_factor; 
      dv2_sum += dv * dv_dep_factor; 
    }
  }
  dr += vdv_sum; 
  ddr += dv2_sum; 
}
