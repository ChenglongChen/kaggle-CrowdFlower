/* * * * *
 *  AzReg_Tsrbase.cpp 
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

#include "AzReg_Tsrbase.hpp"
#include "AzPrint.hpp"

/*
 * To evaluate node split: 
 *   Assume two new leaf nodes under the node on focus (the node to be split)
 *   with the weights same as the node on focus, so that predictions stay the same. 
 */

/*--------------------------------------------------------*/
/**
  *  dr=sum_j v_j dv_j; ddr=sum_j dv_j^2
  *  v_j = bar_j - bar_{p(j)} where p(j) is j's parent. 
  *
  *  output: newleaf_v, newleaf_dep_factor, focus_dbar, 
  *          vdv_sum, dv2_dum, dr, ddr
 **/
void AzReg_Tsrbase::update()
{
  /* vdv_sum, dv2_sum: don't include new leaf nodes */
  /* dr, ddr:           include new leaf nodes */

  dr = ddr = 0; 

  if (forNewLeaf) {
    checkLeaf("update"); 

    /*---  new leaf  ---*/
    newleaf_v = get_newleaf_v(); 
    double dv = get_newleaf_dv(); 
    int newleaf_depth = get_newleaf_depth(); 
    newleaf_dep_factor = reg_depth->apply(1, newleaf_depth); 
    dr += newleaf_v*dv*newleaf_dep_factor; 
    ddr += dv*dv*newleaf_dep_factor; 

    /*---  new leaf's sibling (v is the same)  ---*/
    dv = get_newleaf_sib_dv(); 
    dr += newleaf_v*dv*newleaf_dep_factor; 
    ddr += dv*dv*newleaf_dep_factor; 

    focus_dbar = av_dbar.point(focus_nx)->get(focus_nx); 
  }

  vdv_sum = 0;  
  int nx; 
  if (v_v.rowNum() > 0) {
    /*---  for efficiency don't use get_v and get_dv  ---*/
    const double *v_arr = v_v.point(); 
    if (av_dv.size() <= 0) {
      throw new AzException("AzReg_Tsrbase::update", "no dv?"); 
    }
    const double *dv_arr = av_dv.point(focus_nx)->point(); 

    for (nx = 0; nx < tree->nodeNum(); ++nx) {
      double v = v_arr[nx]; 
      double dv = dv_arr[nx]; 
      int depth = tree->node(nx)->depth; 
      vdv_sum += reg_depth->apply(v * dv, depth); 
    } 
  }  
  else {
    for (nx = 0; nx < tree->nodeNum(); ++nx) {
      double v = get_v(nx); 
      double dv = get_dv(nx); 
      int depth = tree->node(nx)->depth; 
      vdv_sum += reg_depth->apply(v * dv, depth); 
    } 
  }
  dv2_sum = get_dv2_sum(); 

  dr += vdv_sum; 
  ddr += dv2_sum; 
}

/*--------------------------------------------------------*/
double AzReg_Tsrbase::penalty()
const
{
  double r = 0; 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    double v = get_v(nx); 
    r += reg_depth->apply(v*v/2, tree->node(nx)->depth); 
  }

  if (forNewLeaf) {
    checkLeaf("penalty"); 
    double v = get_newleaf_v(); 
    double rr = 2*reg_depth->apply(v*v/2, get_newleaf_depth()); /* 2* for siblings */
    r += rr; 
  }

  return r; 
}

/*--------------------------------------------------------*/
//! only called from OptOnTree 
void AzReg_Tsrbase::changeWeight(int nx, double w_diff)
{
  if (w_diff != 0) {
    if (v_v.rowNum() > 0) { /* called from OptOnTree */
      v_v.add(av_dv.point(nx), w_diff); 
    }
    else {
      v_bar.add(av_dbar.point(nx), w_diff); 
    }
  }
}

/*--------------------------------------------------------*/
/*
 * For fast training, set the following in update() in advance: 
 *      focus_delta, nonleaf_dep_factor, and nonleaf_v. 
 */
double AzReg_Tsrbase::penalty_diff(const double leaf_w_delta[2])
const
{
  double diff = penalty_offset; 

  /*---  on the existing nodes  ---*/
  double delsum = leaf_w_delta[0] + leaf_w_delta[1]; 
  diff += vdv_sum * delsum; 
  diff += (dv2_sum * (delsum*delsum) * 0.5); 

  /*---  on the new leaf nodes  ---*/
  double focus_delta = focus_dbar*delsum; 
  double d0 = leaf_w_delta[0] - focus_delta; 
  double d1 = leaf_w_delta[1] - focus_delta; 
  double old_v = newleaf_v; 
  diff += (old_v*(d0+d1)+(d0*d0+d1*d1)/2)*newleaf_dep_factor; 

  return diff; 
}

/*--------------------------------------------------------*/
double AzReg_Tsrbase::get_v(int nx) const
{
  if (v_v.rowNum() > 0) {
    return v_v.get(nx); 
  }
  double v = v_bar.get(nx); 
  int px = tree->node(nx)->parent_nx; 
  if (px >= 0) {
    v -= v_bar.get(px); 
  }
  return v; 
}

/*--------------------------------------------------------*/
double AzReg_Tsrbase::get_dv(int nx) const
{
  checkFocus("get_dv"); 
  if (av_dv.size() > 0) {
    return av_dv.point(focus_nx)->get(nx); 
  }

  const AzDvect *v_dbar = av_dbar.point(focus_nx); 
  double dv = v_dbar->get(nx); 
  int px = tree->node(nx)->parent_nx; 
  if (px >= 0) {
    dv -= v_dbar->get(px); 
  }
  return dv; 
}

/*--------------------------------------------------------*/
double AzReg_Tsrbase::get_dv2_sum() const
{
  checkFocus("get_dv2_sum"); 

  if (v_dv2_sum.rowNum() > 0) {
    return v_dv2_sum.get(focus_nx); 
  }
  double my_dv2_sum = 0; 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    double dv = get_dv(nx); 
    int depth = tree->node(nx)->depth; 
    my_dv2_sum += reg_depth->apply(dv * dv, depth); 
  }  
  return my_dv2_sum; 
}

/*--------------------------------------------------------*/
void AzReg_Tsrbase::penalty_deriv(int nx, double *dr, 
                                  double *ddr) 
{
  focus_nx = nx; 
  update(); 
  penalty_deriv(dr, ddr); 
}

/*--------------------------------------------------------*/
void AzReg_Tsrbase::penalty_deriv(double *out_dr, 
                             double *out_ddr) const 
{
  *out_dr = dr; 
  *out_ddr = ddr; 
}

