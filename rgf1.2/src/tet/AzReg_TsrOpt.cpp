/* * * * *
 *  AzReg_TsrOpt.cpp 
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

#include "AzReg_TsrOpt.hpp"
#include "AzHelp.hpp"

/*
 * To evaluate node split: 
 *   Assume two new leaf nodes under the node on focus (the node to be split)
 *   with the weights same as the node on focus, so that predictions stay the same. 
 */

#define coeff_sum_index 3

/*--------------------------------------------------------*/
void AzReg_TsrOpt::resetTreeStructure()
{
  iia_le_gt.reset(); 
}

/*--------------------------------------------------------*/
/* static */
void AzReg_TsrOpt::storeTreeStructure()
{
  iia_le_gt.reset(); 
  iia_le_gt.prepare(tree->nodeNum()); 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    iia_le_gt.put(tree->node(nx)->le_nx, 
                  tree->node(nx)->gt_nx); 
  }
}

/*--------------------------------------------------------*/
/* static */
bool AzReg_TsrOpt::isSameTreeStructure() const
{
  if (tree->nodeNum() != iia_le_gt.size()) {
    return false; 
  }
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    int le_nx, gt_nx; 
    iia_le_gt.get(nx, &le_nx, &gt_nx); 
    if (tree->node(nx)->le_nx != le_nx || 
        tree->node(nx)->gt_nx != gt_nx) {
      return false; 
    }
  }  

  return true; 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::_reset(const AzTrTree_ReadOnly *inp_tree, 
                          const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  reg_depth = inp_reg_depth; 

  if (tree == NULL) {
    throw new AzException("AzReg_TsrOpt::_reset", "null tree"); 
  }

  bool isSame = isSameTreeStructure(); 

  forNewLeaf = false; 
  int node_num = tree->nodeNum(); 
  v_bar.reform(node_num);  
  if (!isSame) {
    /*---  new tree structre  ---*/
    av_dbar.reset(node_num);  
  }
  else {
    /*---  same tree structre as before; reuse dbar  ---*/
    if (av_dbar.size() != node_num || 
        av_dv.size() != node_num || 
        v_dv2_sum.rowNum() != node_num) {
      throw new AzException("AzReg_TsrOpt::reset", 
                "tree structure is same but other info doesn't match"); 
    }
  }

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }

  int ix; 
  for (ix = 0; ix < ia_leaf.size(); ++ix) {
    nx = ia_leaf.get(ix); 
    AzDvect *v_dbar = av_dbar.point_u(nx); 

    if (!isSame) {
      deriv(nx, &ia_nonleaf, v_dbar); 
    }

    double w = tree->node(nx)->weight; 
    v_bar.add(v_dbar, w); 
    v_bar.set(nx, w); 
  }  

  focus_nx = -1; 
  if (!isSame) {
    update_dv(); 
    storeTreeStructure();   
  }
  update_v(); 
}

/*--------------------------------------------------------*/
/* update v_dv and v_dv2_sum */
void AzReg_TsrOpt::update_dv()
{
  av_dv.reset(tree->nodeNum());  
  v_dv2_sum.reform(tree->nodeNum()); 
  int f_nx; 
  for (f_nx = 0; f_nx < tree->nodeNum(); ++f_nx) {
    if (!tree->node(f_nx)->isLeaf()) continue; 

    const AzDvect *v_dbar = av_dbar.point(f_nx); 
    AzDvect *v_dv = av_dv.point_u(f_nx); 
    v_dv->reform(tree->nodeNum()); 
    double dv2_sum = 0; 
    int nx; 
    for (nx = 0; nx < tree->nodeNum(); ++nx) {
      double dv = v_dbar->get(nx); 
      int px = tree->node(nx)->parent_nx; 
      if (px >= 0) {
        dv -= v_dbar->get(px); 
      }
      v_dv->set(nx, dv); 

 
      int depth = tree->node(nx)->depth; 
      dv2_sum += reg_depth->apply(dv*dv, depth); 
    }
    v_dv2_sum.set(f_nx, dv2_sum); 
  }
}

/*--------------------------------------------------------*/
/* update v_v */
void AzReg_TsrOpt::update_v()
{
  v_v.reform(tree->nodeNum()); 
  int nx; 
  for (nx = 0; nx < tree->nodeNum(); ++nx) {
    double v = v_bar.get(nx); 
    int px = tree->node(nx)->parent_nx; 
    if (px >= 0) {
      v -= v_bar.get(px); 
    }
    v_v.set(nx, v); 
  }
  v_bar.reset(); 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::_reset_forNewLeaf(const AzTrTree_ReadOnly *inp_tree, 
                                     const AzRegDepth *inp_reg_depth) 
{
  tree = inp_tree; 
  forNewLeaf = false; 
  focus_nx = -1; 
  reg_depth = inp_reg_depth; 
  resetTreeStructure(); 
  reset_v_dv(); 
  
  int node_num = tree->nodeNum(); 

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }
  int split_nx = -1; 
  reset_bar(split_nx, &ia_leaf, &ia_nonleaf);

  curr_penalty = AzReg_Tsrbase::penalty();
}

/*--------------------------------------------------------*/
double AzReg_TsrOpt::_reset_forNewLeaf(int inp_focus_nx, 
                      const AzTrTree_ReadOnly *inp_tree, 
                      const AzRegDepth *inp_reg_depth)
{
  tree = inp_tree; 
  forNewLeaf = true; 
  focus_nx = inp_focus_nx; 
  reg_depth = inp_reg_depth; 
  resetTreeStructure(); 
  reset_v_dv(); 
  
  int node_num = tree->nodeNum(); 

  AzIntArr ia_nonleaf, ia_leaf; 
  int nx; 
  for (nx = 0; nx < node_num; ++nx) {
    if (!tree->node(nx)->isLeaf()) ia_nonleaf.put(nx); 
    else                           ia_leaf.put(nx); 
  }

  av_dbar.reset(node_num); 
  AzDvect *v_dbar = av_dbar.point_u(focus_nx); 

  deriv(focus_nx, &ia_nonleaf, v_dbar); 
  int split_nx = focus_nx; 
  reset_bar(split_nx, &ia_leaf, &ia_nonleaf); 

  double penalty_offset = AzReg_Tsrbase::penalty() - curr_penalty; 
  return penalty_offset; 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::reset_bar(int split_nx, 
                      const AzIntArr *ia_leaf, 
                      const AzIntArr *ia_nonleaf)
{
  v_bar.reform(tree->nodeNum()); 

  int ix; 
  for (ix = 0; ix < ia_leaf->size(); ++ix) {
    int nx = ia_leaf->get(ix); 
    v_bar.set(nx, tree->node(nx)->weight); 
  }

  double new_leaf_w[2] = {0, 0}; 
  if (split_nx >= 0) {
    /** 
      *  this is for evaluating node split.  pretend that the node 
      *  to be split is an internal node and there are two new leaves 
      *  under it whose weights are the weight of the node to be split.  
     **/
    v_bar.set(split_nx, 0); 
    double w = tree->node(split_nx)->weight; 
    new_leaf_w[0] = new_leaf_w[1] = w; 
  }

  _propagate(reg_ite_num, tree, split_nx, new_leaf_w, ia_nonleaf, reg_depth, 
             m_coeff, &v_bar); 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::deriv(int base_nx, 
                         const AzIntArr *ia_nonleaf, 
                         AzDvect *out_v_dbar)
{
  out_v_dbar->reform(tree->nodeNum()); 

  int split_nx = -1; 
  double new_leaf_w[2] = {0,0}; 
  if (!forNewLeaf) {
    out_v_dbar->set(base_nx, 1); 
  }
  else {
    new_leaf_w[0] = 1; 
    split_nx = base_nx; 
  }

  _propagate(reg_ite_num, tree, split_nx, new_leaf_w, ia_nonleaf, reg_depth, 
             m_coeff, out_v_dbar); 
}

/*--------------------------------------------------------*/
/* static */
void AzReg_TsrOpt::_propagate(int ite_num, 
                            const AzTrTree_ReadOnly *tree, 
                            int split_nx, 
                            const double new_leaf_w[2], 
                            const AzIntArr *ia_nonleaf, 
                            const AzRegDepth *reg_depth, 
                            AzDmat *m_coeff, /* inout */
                            AzDvect *v)
{
  const char *eyec = "AzReg_TsrOpt::propagate"; 

  setCoeff(reg_depth, tree, m_coeff); 

  if (split_nx >= 0 && !tree->node(split_nx)->isLeaf()) {
    throw new AzException(eyec, "node to be split must be a leaf"); 
  }

  const int *nonleaf = ia_nonleaf->point(); 
  double *v_arr = v->point_u(); 
  if (v->rowNum() != tree->nodeNum()) {
    throw new AzException(eyec, "v's dim is wrong"); 
  }

  double split_fixed_sum=0, split_coeff_sum=0, split_coeff0=0; 
  int split_px = -1; 
  if (split_nx >= 0) {
    const AzTrTreeNode *np = tree->node(split_nx); 
    const double *coeff = m_coeff->col(np->depth)->point(); 
    split_coeff0 = coeff[0]; 
    split_coeff_sum = coeff[coeff_sum_index]; 
    split_fixed_sum = coeff[1]*new_leaf_w[0] + coeff[2]*new_leaf_w[1]; 
    split_px = np->parent_nx;  
  }

  int real_nonleaf_num = ia_nonleaf->size(); 
  if (split_nx >= 0) {
    ++real_nonleaf_num; 
  }

  int ite; 
  for (ite = 0; ite < ite_num; ++ite) {
    if (split_nx >= 0) {
      double sum = split_fixed_sum; 
      if (split_px >= 0) {
        sum += split_coeff0*v_arr[split_px]; 
      }
      sum /= split_coeff_sum; 
      v_arr[split_nx] = sum; 
    }

    int ix; 
    for (ix = 0; ix < ia_nonleaf->size(); ++ix) {
      int nx = nonleaf[ix]; 
      const AzTrTreeNode *np = tree->node(nx); 

      const double *coeff = m_coeff->col(np->depth)->point(); 
      double new_v = 0; 
      if (np->parent_nx >= 0) {
        new_v += coeff[0]*v_arr[np->parent_nx]; 
      }
      new_v += coeff[1]*v_arr[np->le_nx]; 
      new_v += coeff[2]*v_arr[np->gt_nx]; 
      new_v /= coeff[coeff_sum_index]; 

      v_arr[nx] = new_v; 
    }

    if (real_nonleaf_num <= 1) {
      break; /* since iteration won't change the results */
    }
  }
}


/*--------------------------------------------------------*/
/* static */
void AzReg_TsrOpt::setCoeff(const AzRegDepth *reg_depth, 
                            const AzTrTree_ReadOnly *tree, 
                            AzDmat *m_coeff)
{
  if (m_coeff == NULL) {
    throw new AzException("AzReg_TsrOpt::setCoeff", "m_coeff=null"); 
  }

  int max_depth = tree->nodeNum(); /* upper bound */
  if (m_coeff->colNum() >= max_depth+1) {
    return; 
  }
  int old_max_depth = m_coeff->colNum()-1; 
  max_depth = MAX(max_depth, 50); 
  m_coeff->resize(coeff_sum_index+1, max_depth+1); 
  int depth; 
  for (depth = old_max_depth+1; depth <= max_depth; ++depth) {
    double *coeff = m_coeff->col_u(depth)->point_u(); 
    setCoeff(reg_depth, depth, coeff); 
  }
}

/*--------------------------------------------------------*/
/* static */
void AzReg_TsrOpt::setCoeff(const AzRegDepth *reg_depth, 
                            int depth, 
                            double coeff[4])
{
  coeff[0] = coeff[1] = coeff[2] = 1; 
  if (reg_depth != NULL) {
    /*!!!  assuming c^depth  */
    double lam_nx = reg_depth->apply(1, depth); 
    if (lam_nx == 0) {
      coeff[0] = 0; 
    }
    else {
      double lam_c = reg_depth->apply(1, depth+1); 
      coeff[0] = 1; 
      coeff[1] = lam_c / lam_nx; 
      coeff[2] = coeff[1]; 
    }
  }
  coeff[coeff_sum_index] = coeff[0]+coeff[1]+coeff[2]; 
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzReg_TsrOpt::show(const AzOut &out, 
                    const char *header, 
                    const AzRegDepth *reg_depth, 
                    int focus_nx, 
					const AzDataArray<AzDvect> *av_dbar, 
                    const AzTrTree_ReadOnly *tree, 
                    const AzDvect *v_bar)
{
  if (out.isNull()) return; 
  AzPrint o(out); 
  o.printBegin(header, ", ", "="); 
  o.print("focus_nx", focus_nx); 
  o.printEnd();   

  AzDmat m_coeff; 
  setCoeff(reg_depth, tree, &m_coeff); 

  _show(focus_nx, av_dbar, tree, v_bar, tree->root(), &m_coeff, out); 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::_show(int focus_nx, 
                         const AzDataArray<AzDvect> *av_dbar, 
                         const AzTrTree_ReadOnly *tree, 
                         const AzDvect *v_bar, 
                         int nx, 
                         const AzDmat *m_coeff, 
                         const AzOut &out) 
{
  if (out.isNull()) return; 

  const AzTrTreeNode *np = tree->node(nx); 

  AzPrint o(out); 
  o.printBegin("", ", ", "=", np->depth*2); 
  o.inBrackets(nx,3); 

  o.print("bar", v_bar->get(nx), 4); 
  double v = v_bar->get(nx); 
  if (np->parent_nx >= 0) {
    v -= v_bar->get(np->parent_nx); 
  }
  o.print("v", v, 4); 
  if (focus_nx >= 0) {
    o.print("dbar", av_dbar->point(focus_nx)->get(nx)); 
  }

  if (np->isLeaf()) {
    o.inParen(np->weight, 4); 
  }
  o.printEnd(); 

  if (!np->isLeaf()) {
    _show(focus_nx, av_dbar, tree, v_bar, np->le_nx, m_coeff, out); 
    _show(focus_nx, av_dbar, tree, v_bar, np->gt_nx, m_coeff, out); 
  }
}

/*--------------------------------------------------------*/
/*--------------------------------------------------------*/
void AzReg_TsrOpt::printHelp(AzHelp &h) const
{
  h.begin(Azopt_config, "AzReg_TsrOpt", "Min-penalty regularization"); 
  h.item_experimental(kw_reg_ite_num, help_reg_ite_num, reg_ite_num_dflt); 
  h.end(); 
}

/*------------------------------------------------------------------*/
void AzReg_TsrOpt::resetParam(AzParam &p)
{
  p.vInt(kw_reg_ite_num, &reg_ite_num); 
}

/*--------------------------------------------------------*/
void AzReg_TsrOpt::printParam(const AzOut &out) const 
{
  if (out.isNull()) return; 

  AzPrint o(out); 
  o.ppBegin("AzReg_TsrOpt", "Min-penalty regularization", ", "); 
  o.printV(kw_reg_ite_num, reg_ite_num); 
  o.ppEnd(); 
}
