/* * * * *
 *  AzTrTree.cpp 
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

#include "AzTrTree.hpp"
#include "AzTools.hpp"
#include "AzPrint.hpp"

/*--------------------------------------------------------*/
void AzTrTree::_release()
{
  ia_root_dx.reset(); 
  a_node.free(&nodes); nodes_used = 0; 
  a_split.free(&split); 
  a_sorted_arr.free(&sorted_arr); 

  root_nx = AzNone; 
  curr_min_pop = curr_max_depth = -1; 
}

/*--------------------------------------------------------*/
void AzTrTree::_releaseWork()
{
  a_split.free(&split); 
  a_sorted_arr.free(&sorted_arr);
}

/*--------------------------------------------------------*/
double AzTrTree::getRule(int inp_nx, 
                        AzTreeRule *rule) const
{
  _checkNode(inp_nx, "AzTrTree::getRule"); 
  int child_nx = inp_nx; 
  int nx = nodes[inp_nx].parent_nx; 
  for ( ; ; ) {
    if (nx < 0) break; 

    /*---  feat#, isLE, border_val  ---*/
    bool isLE = false; 
    if (child_nx == nodes[nx].le_nx) {
      isLE = true; 
    }
    rule->append(nodes[nx].fx, isLE, 
                 nodes[nx].border_val); 

    child_nx = nx; 
    nx = nodes[nx].parent_nx; 
  }

  rule->finalize(); 

  return nodes[inp_nx].weight; 
}

/*--------------------------------------------------------*/
void AzTrTree::concat_stat(AzBytArr *o) const 
{
  if (o == NULL) return; 
  int leaf_num = leafNum(); 
  o->c("#node=", nodes_used, 3);
  o->c(",#leaf=", leaf_num, 3);
  o->c(",depth=", curr_max_depth, 2);
  o->c(",min(pop)=", curr_min_pop, 2);
}

/*--------------------------------------------------------*/
void AzTrTree::_genRoot(int max_size, 
                        const AzDataForTrTree *data, 
                        const AzIntArr *ia_dx)
{
  _release(); 

  /*---  generate the root  --*/
  root_nx = _newNode(max_size); 
  AzTrTreeNode *root_np = &nodes[root_nx]; 

  root_np->depth = 0; 
  if (ia_dx == NULL) {
    int data_num = data->dataNum(); 
    ia_root_dx.range(0, data_num); 
  }
  else {
    ia_root_dx.reset(ia_dx); 
  }
  root_np->dxs = ia_root_dx.point(&root_np->dxs_num); 
  root_np->dxs_offset = 0; 
}

/*--------------------------------------------------------*/
int AzTrTree::_newNode(int max_size)
{
  const char *eyec = "AzTrTree::_newNode"; 

  int node_no = nodes_used; 
  int node_max = a_node.size(); 

  if (nodes_used >= node_max) {
    int inc = MAX(node_max, 128); 
    if (max_size > 0) {
      inc = MIN(inc, max_size*2); 
    }
    node_max += inc; 
    a_node.realloc(&nodes, node_max, eyec, "node"); 
    a_split.realloc(&split, node_max, eyec, "split"); 
    a_sorted_arr.realloc(&sorted_arr, node_max, eyec, "sorted_arr"); 
  } 
  else {
    /*---  initialize the new node  ---*/
    nodes[node_no].reset();
  }
  ++nodes_used; 
  return node_no; 
}

/*--------------------------------------------------------*/
void AzTrTree::show(const AzSvFeatInfo *feat, const AzOut &out) const
{
  if (out.isNull()) return; 
  if (nodes == NULL) {
    AzPrint::writeln(out, "AzTrTree::show, No tree"); 
    return; 
  }
  _show(feat, root_nx, 0, out); 
}

/*--------------------------------------------------------*/
void AzTrTree::_show(const AzSvFeatInfo *feat, 
                    int nx, 
                    int depth, 
                    const AzOut &out) const
{
  if (out.isNull()) return; 

  const AzTrTreeNode *np = &nodes[nx]; 
  int pop = np->dxs_num; 

  AzPrint o(out); 
  o.printBegin("", ", ", "=", depth*2); 
  /* [nx], (pop,weight), depth=d, desc,border */
  if (np->isLeaf()) {
    o.print("*"); 
    o.disableDlm(); 
  }
  o.inBrackets(nx,3); 
  o.enableDlm(); 
  if (pop >= 0 || np->weight != 0) {
    AzBytArr s_pop_weight; 
    if (pop >= 0) s_pop_weight.cn(pop,3); 
    s_pop_weight.c(", "); 
    if (np->weight != 0) s_pop_weight.cn(np->weight,3); 
    o.inParen(s_pop_weight); 
  }

  o.printV("depth=", depth); 
  if (np->fx >= 0 && feat != NULL) {
    AzBytArr s_desc; 
    feat->concatDesc(np->fx, &s_desc); 
    o.print(&s_desc); 
    o.print(np->border_val,4,false); 
  }
  o.printEnd(); 

  if (!np->isLeaf()) {
    _show(feat, np->le_nx, depth+1, out); 
    _show(feat, np->gt_nx, depth+1, out); 
  }
}

/*--------------------------------------------------------*/
void AzTrTree::_splitNode(const AzDataForTrTree *data, 
                        int max_size, 
                        bool doUseInternalNodes, 
                        int nx,
                        const AzTrTsplit *inp, 
                        const AzOut &out)
{
  _checkNode(nx, "AzTrTree::splitNode"); 

  nodes[nx].fx = inp->fx; 
  nodes[nx].border_val = inp->border_val; 

  AzIntArr ia_le, ia_gt; 
  const AzSortedFeatArr *s_arr = sorted_arr[nx]; 
  if (s_arr == NULL) {
    if (nx == root_nx) {
      s_arr = data->sorted_array(); 
    }
    else {
      throw new AzException("AzTrTree::_splitNode", "sorted_arr[nx]=null"); 
    }
  }
  const AzSortedFeat *sorted = s_arr->sorted(inp->fx); 
  if (sorted == NULL) {
    AzSortedFeatWork tmp; 
    const AzSortedFeat *my_sorted = sorted_arr[nx]->sorted(data->sorted_array(), 
                                    inp->fx, &tmp); 
    my_sorted->getIndexes(nodes[nx].dxs, nodes[nx].dxs_num, inp->border_val, 
                          &ia_le, &ia_gt); 
  }
  else {
    sorted->getIndexes(nodes[nx].dxs, nodes[nx].dxs_num, inp->border_val, 
                       &ia_le, &ia_gt); 
  }

  int le_offset = nodes[nx].dxs_offset; 
  int gt_offset = le_offset + ia_le.size(); 

  int le_nx = _newNode(max_size); 
  nodes[nx].le_nx = le_nx; 
  AzTrTreeNode *np = &nodes[le_nx]; 
  np->depth = nodes[nx].depth + 1;
  np->dxs_offset = le_offset; 
  np->dxs = set_data_indexes(le_offset, ia_le.point(), ia_le.size()); 
  np->dxs_num = ia_le.size(); 
  np->parent_nx = nx; 
  np->weight = inp->bestP[0]; 
  if (curr_min_pop < 0 || np->dxs_num < curr_min_pop) curr_min_pop = np->dxs_num; 
  curr_max_depth = MAX(curr_max_depth, np->depth); 

  int gt_nx = _newNode(max_size);   
  nodes[nx].gt_nx = gt_nx; 
  np = &nodes[gt_nx]; 
  np->depth = nodes[nx].depth + 1; 
  np->dxs_offset = gt_offset; 
  np->dxs = set_data_indexes(gt_offset, ia_gt.point(), ia_gt.size()); 
  np->dxs_num = ia_gt.size(); 
  np->parent_nx = nx; 
  np->weight = inp->bestP[1]; 
  curr_min_pop = MIN(curr_min_pop, np->dxs_num); 

  /*------------------------------*/
  double org_weight = nodes[nx].weight; 
  if (!doUseInternalNodes) {
    nodes[nx].weight = 0; 
  }

  /*------------------------------*/
  dump_split(inp, nx, org_weight, out); 

  /*---  release split info for the node we just split  ---*/
  delete split[nx]; split[nx] = NULL; 
}

/*--------------------------------------------------------*/
void AzTrTree::dump_split(const AzTrTsplit *inp, 
                     int nx, 
                     double org_weight, 
                     const AzOut &out)
{
  if (out.isNull()) return; 

  _checkNode(nx, "AzTrTree::dump_split"); 
  int gt_nx = nodes[nx].gt_nx; 
  int le_nx = nodes[nx].le_nx; 

  AzPrint o(out); 
  o.printBegin("", ", ", "="); 
  if (inp->tx >= 0) {
    o.pair_inBrackets(inp->tx, nx, ":"); 
  }
  else {
    o.inBrackets(nx); 
  }
  o.print("d", nodes[nx].depth); 
  o.print("fx", nodes[nx].fx); o.print(nodes[nx].border_val, 5); 

  o.print(nodes[nx].dxs_num); 
  o.disableDlm(); 
  o.print("->"); o.pair_inParen(nodes[le_nx].dxs_num, nodes[gt_nx].dxs_num, ","); 
  o.enableDlm(); 

  o.print(org_weight,4); 
  o.disableDlm(); 
  o.print("->"); 
  o.set_precision(4); 
  o.pair_inParen(nodes[le_nx].weight, nodes[gt_nx].weight, ","); 
  o.enableDlm(); 

  o.print("gain", inp->gain, 5); 
  o.print(inp->str_desc.c_str()); 
  o.printEnd(); 
  o.flush(); 
}

/*--------------------------------------------------------*/
bool AzTrTree::isEmptyTree() const
{
  if (nodes_used == 1 && 
      nodes[0].weight == 0) {
    return true; 
  }
  return false; 
}

/*--------------------------------------------------------*/
int AzTrTree::leafNum() const
{
  _checkNodes("AzTrTree::leafNum"); 
  int leaf_num = 0; 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) {
      ++leaf_num; 
    }
  }
  return leaf_num; 
}

/*--------------------------------------------------------*/
void AzTrTree::concatDesc(const AzSvFeatInfo *feat, 
                      int nx, 
                      AzBytArr *str_desc, /* output */
                      int max_len) const
{
  if (feat == NULL) {
    str_desc->concat("not_available"); 
  }

  _checkNode(nx, "AzTrTree::concatDesc"); 
  if (nx == root_nx) {
    str_desc->concat("ROOT"); 
  }
  else {
    _genDesc(feat, nx, str_desc); 
  }

  if (max_len > 0 && str_desc->getLen() > max_len) {
    AzBytArr str(str_desc->point(), max_len); 
    str.concat("..."); 
    str_desc->clear(); 
    str_desc->concat(&str); 
  }
}

/*--------------------------------------------------------*/
void AzTrTree::_genDesc(const AzSvFeatInfo *feat, 
                       int nx, 
                       AzBytArr *str_desc) /* output */
					   const
{
  int px = nodes[nx].parent_nx; 
  if (px < 0) return; 

  _genDesc(feat, px, str_desc); 
  if (str_desc->getLen() > 0) {
    str_desc->concat(";"); 
  }
  feat->concatDesc(nodes[px].fx, str_desc); 
  if (nodes[px].le_nx == nx) {
    str_desc->concat("<="); 
  }
  else {
    str_desc->concat(">"); 
  }
  /* sprintf(work, "%5.3f", nodes[px]->border_val); */
  str_desc->concatFloat(nodes[px].border_val, 5); 
}

/*--------------------------------------------------------*/
void AzTrTree::_isActiveNode(bool doWantInternalNodes, 
                                 bool doAllowZeroWeightLeaf, 
                                 AzIntArr *ia_isActiveNode) const
{
  _checkNodes("AzTree_Reggfo::isActiveNode"); 
  ia_isActiveNode->reset(nodes_used, false); 
  int *isActive = ia_isActiveNode->point_u(); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    if (nodes[nx].isLeaf()) {
      if (nodes[nx].weight != 0) {
        isActive[nx] = true; 
      }
      else {  /* zero-weight leaf */
        if (doAllowZeroWeightLeaf) {
          isActive[nx] = true; 
        }
        else {
          isActive[nx] = false; 
        }
      }
    }
    else if (doWantInternalNodes && nx != root_nx) {
      isActive[nx] = true; 
    }
  }
}

/*--------------------------------------------------------*/
void AzTrTree::apply(const AzDataForTrTree *data, 
                     const AzIntArr *ia_dx, 
                     AzDvect *v_p) /* output */
const
{
  v_p->reform(data->dataNum()); 
  double *p = v_p->point_u(); 
  int num; 
  const int *dxs = ia_dx->point(&num); 
  int ix; 
  for (ix = 0; ix < num; ++ix) {
    int dx = dxs[ix]; 
    p[dx] = apply(data, dx, NULL); 
  }
}

/*--------------------------------------------------------*/
double AzTrTree::apply(const AzDataForTrTree *data, 
                       int dx, 
                       AzIntArr *ia_nx) /* node path */
const 
{
  const char *eyec = "AzTrTree::apply(dfd,dx)"; 
  int nx = root_nx; 
  double p_val = 0; 
  for ( ; ; ) {
    if (nx < 0) {
      throw new AzException(eyec, "stuck"); 
    }
    if (ia_nx != NULL) {
      ia_nx->put(nx); 
    }
    const AzTrTreeNode *np = &nodes[nx]; 
    p_val += np->weight; 
    if (np->isLeaf()) { /* leaf */
      break; 
    }

    bool isLE = data->isLE(dx, np->fx, np->border_val); 
    if (isLE) {
      nx = nodes[nx].le_nx;         
    }
    else {
      nx = nodes[nx].gt_nx; 
    }
  }
  return p_val; 
} 

/*------------------------------------------------------------------*/
void AzTrTree::updatePred(const AzDataForTrTree *dfd, 
                          AzDvect *v_pval) /* inout */
const
{
  int data_num = dfd->dataNum(); 
  if (v_pval->rowNum() != data_num) {
    throw new AzException("AzTrTree::updatePred", "dim mismatch"); 
  }
  double *p_val = v_pval->point_u(); 
  int dx; 
  for (dx = 0; dx < data_num; ++dx) {
    p_val[dx] += apply(dfd, dx); 
  }
}

/*--------------------------------------------------------*/
void AzTrTree::copy_nodes_from(const AzTrTree_ReadOnly *inp) 
{
  const char *eyec = "AzTrTree::copy_nodes_from"; 
  _release(); 
  root_nx = inp->root(); 
  nodes_used = inp->nodeNum(); 
  ia_root_dx.reset(inp->root_dx()); 
  const int *root_dxs = ia_root_dx.point(); 
  a_node.alloc(&nodes, nodes_used, eyec); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx] = *inp->node(nx); 
    if (inp->node(nx)->dxs != NULL && ia_root_dx.size() > 0) {
      nodes[nx].dxs = root_dxs + nodes[nx].dxs_offset; 
    }
  }
}

/*--------------------------------------------------------*/
const int *AzTrTree::set_data_indexes(int offset, 
                     const int *dxs, 
                     int dxs_num)
{
  int ix_end = offset + dxs_num; 
  if (ix_end > ia_root_dx.size()) {
    throw new AzException("AzTrTree::set_data_indexes", "out of range"); 
  } 

  int *root_dxs = ia_root_dx.point_u(); 
  int ix; 
  for (ix = 0; ix < dxs_num; ++ix) {
    root_dxs[ix+offset] = dxs[ix]; 
  }
  return root_dxs + offset; 
}

/*--------------------------------------------------------*/
void AzTrTree::warmup(const AzTreeNodes *inp, 
                      const AzDataForTrTree *data, 
                      AzDvect *v_p, /* inout */
                      const AzIntArr *ia_tr_dx) /* may be NULL */
{
  const char *eyec = "AzTrTree::warmup"; 
  _release(); 

  if (inp->nodeNum() == 0) {
    return; 
  }

  AzIntArr ia_split_nx; 
  int nx; 
  for (nx = 0; nx < inp->nodeNum(); ++nx) {
    const AzTreeNode *inp_np = inp->node(nx); 
    if (inp_np->parent_nx >= 0 && inp->node(inp_np->parent_nx)->le_nx == nx) {
      ia_split_nx.put(inp_np->parent_nx); 
    }
    if (!inp_np->isLeaf() && inp_np->weight != 0) { /* this shouldn't happen, though */
      throw new AzException(eyec, "internal nodes have non-zero weights"); 
    }
  }

  _genRoot(inp->nodeNum(), data, ia_tr_dx); 
  if (root_nx != inp->root()) {
    throw new AzException(eyec, "root node id mismatch"); 
  }

  AzOut dummy_out; 
  dummy_out.deactivate(); 
  int ix; 
  for (ix = 0; ix < ia_split_nx.size(); ++ix) {
    int split_nx = ia_split_nx.get(ix); 
    if (split_nx >= nodes_used) {
      throw new AzException(eyec, "something is wrong with split order"); 
    }

    sorted_array(split_nx, data); 

    const AzTreeNode *inp_np = inp->node(split_nx); 
    double dummy_gain = 1.0; 
    AzTrTsplit split(inp_np->fx, inp_np->border_val, dummy_gain, 
                     inp->node(inp_np->le_nx)->weight, 
                     inp->node(inp_np->gt_nx)->weight);  
    _splitNode(data, inp->nodeNum(), false, split_nx, &split, dummy_out); 
  }

  /*---  compare the basic structure  ---*/
  if (nodes_used != inp->nodeNum()) {
    throw new AzException(eyec, "#node mismatch"); 
  }

  double *p = v_p->point_u(); 
  for (nx = 0; nx < nodes_used; ++nx) {
    const AzTreeNode *inp_np = inp->node(nx); 
    AzTrTreeNode *np = &nodes[nx]; 
    if (np->fx != inp_np->fx || 
        np->border_val != inp_np->border_val || 
        np->le_nx != inp_np->le_nx || 
        np->gt_nx != inp_np->gt_nx || 
        np->parent_nx != inp_np->parent_nx || 
        np->weight != inp_np->weight) {
      throw new AzException(eyec, "conflict in basic structure"); 
    } 

    /*---  update predictions  ---*/
    if (np->weight != 0) {
      int ix; 
      for (ix = 0; ix < np->dxs_num; ++ix) {
        int dx = np->dxs[ix]; 
        p[dx] += np->weight; 
      }
    }
  }
}

/*--------------------------------------------------------*/
void AzTrTree::quick_warmup(const AzTreeNodes *inp, 
                      const AzDataForTrTree *data, 
                      AzDvect *v_p, /* inout */
                      const AzIntArr *inp_ia_tr_dx) /* may be NULL */
{
  const char *eyec = "AzTrTree::warmup"; 
  _release(); 

  const AzIntArr *ia_tr_dx = inp_ia_tr_dx; 
  AzIntArr ia_temp; 
  if (ia_tr_dx == NULL) {
    ia_temp.range(0, data->dataNum()); 
    ia_tr_dx = &ia_temp; 
  }

  /*---  copy the basic structure  ---*/
  root_nx = inp->root(); 
  nodes_used = inp->nodeNum(); 
  a_node.alloc(&nodes, nodes_used, eyec, "nodes"); 
  a_split.free(&split); 
  a_sorted_arr.free(&sorted_arr); 
  int nx; 
  for (nx = 0; nx < nodes_used; ++nx) {
    const AzTreeNode *inp_np = inp->node(nx); 
    AzTrTreeNode *out_np = &nodes[nx]; 
    out_np->fx         = inp_np->fx; 
    out_np->border_val = inp_np->border_val; 
    out_np->le_nx      = inp_np->le_nx; 
    out_np->gt_nx      = inp_np->gt_nx; 
    out_np->parent_nx  = inp_np->parent_nx; 
    out_np->weight     = inp_np->weight; 

    if (!inp_np->isLeaf() && inp_np->weight != 0) { /* this shouldn't happen, though */
      throw new AzException(eyec, "internal nodes have non-zero weights"); 
    }
  }

  /*---  set data points  ---*/
  AzDataArray<AzIntArr> aIa_dx(nodes_used); 
  int dx_num = ia_tr_dx->size(); 
  int ix; 
  for (ix = 0; ix < dx_num; ++ix) {
    int dx = ia_tr_dx->get(ix); 
    AzIntArr ia_nx; 
    double val = apply(data, dx, &ia_nx); 
    v_p->add(dx, val); 
    int jx; 
    for (jx = 0; jx < ia_nx.size(); ++jx) {
      nx = ia_nx.get(jx); 
      if (nodes[nx].isLeaf()) {
        aIa_dx.point_u(nx)->put(dx); 
      }
    }
  }

  /*---  set node depth and pop ---*/
  for (nx = 0; nx < nodes_used; ++nx) {
    nodes[nx].dxs_num = nodes[nx].depth = 0; 
    nodes[nx].dxs = NULL; 
    nodes[nx].dxs_offset = -1; 
  }
  for (nx = 0; nx < nodes_used; ++nx) {
    int temp_nx = nodes[nx].parent_nx; 
    while(temp_nx >= 0) {
      ++nodes[nx].depth; 
      temp_nx = nodes[temp_nx].parent_nx; 
    }
  }

  /* make ia_root_dx and set dxs_offset, dxs_num, dxs. */
  AzIntArr ia_leaf_in_order; 
  orderLeaves(&ia_leaf_in_order); 

  ia_root_dx.reset(ia_tr_dx->size(), -1); 
  int offset = 0; 
  for (ix = 0; ix < ia_leaf_in_order.size(); ++ix) {
    int leaf_nx = ia_leaf_in_order.get(ix); 
    const AzIntArr *ia_leaf_dx = aIa_dx.point(leaf_nx); 
    nodes[leaf_nx].dxs_offset = offset; 
    nodes[leaf_nx].dxs = set_data_indexes(offset, ia_leaf_dx->point(), 
                                          ia_leaf_dx->size()); 
    nodes[leaf_nx].dxs_num = ia_leaf_dx->size(); 
    offset += ia_leaf_dx->size(); 

    int temp_nx = nodes[leaf_nx].parent_nx; 
    while(temp_nx >= 0) {
      AzTrTreeNode *temp_np = &nodes[temp_nx]; 
      if (temp_np->dxs == NULL) {
        temp_np->dxs_offset = nodes[leaf_nx].dxs_offset; 
        temp_np->dxs = nodes[leaf_nx].dxs; 
      }
      else {
        if (temp_np->dxs_offset+temp_np->dxs_num != nodes[leaf_nx].dxs_offset) {
          throw new AzException(eyec, "Inconsistency in data indexes"); 
        }
      }

      temp_np->dxs_num += nodes[leaf_nx].dxs_num; 
      temp_nx = temp_np->parent_nx; 
    }
  }
  if (offset != ia_tr_dx->size()) {
    throw new AzException(eyec, "conflict in total# of data indexes"); 
  }
}

/*------------------------------------------------------------------*/
void AzTrTree::orderLeaves(AzIntArr *ia_leaf_in_order)
{
  int leaf_num = leafNum(); 
  ia_leaf_in_order->reset(); 
  ia_leaf_in_order->prepare(leaf_num); 
  _orderLeaves(ia_leaf_in_order, root_nx); 
  if (ia_leaf_in_order->size() != leaf_num) {
    throw new AzException("AzTrTree::orderLeaves", "#leaf conflict"); 
  }
}

/*------------------------------------------------------------------*/
void AzTrTree::_orderLeaves(AzIntArr *ia_leaf_in_order, 
                            int nx) 
{
  if (nodes[nx].isLeaf()) {
    ia_leaf_in_order->put(nx); 
    return; 
  }
  _orderLeaves(ia_leaf_in_order, nodes[nx].le_nx); 
  _orderLeaves(ia_leaf_in_order, nodes[nx].gt_nx); 
} 

/*------------------------------------------------------------------*/
double AzTrTree::init_const(AzLossType loss_type, 
                            const AzDvect *v_y, 
                            const AzIntArr *ia_tr_dx) /* may be NULL*/
{
  double y_avg = v_y->average(ia_tr_dx); 
  double const_val = 0; 
  if (loss_type == AzLoss_Logistic2) {
    /* For Algorithm 5 of Friedman'00, Greedy Function Approximation ... */
    const_val = log((1+y_avg)/(1-y_avg))/2; /* e^f/(e^f+e^{-f})=(yavg+1)/2 */
  }
  else if (loss_type == AzLoss_Logistic1) {
    const_val = log((1+y_avg)/(1-y_avg)); /* e^f/(1+e^f)=(yavg+1)/2 */
  }
  else {
    const_val = y_avg; 
  }
  return const_val; 
}

/*------------------------------------------------------------------*/
double AzTrTree::init_constw(AzLossType loss_type, 
                             const AzDvect *v_y, 
                             const AzDvect *v_fixed_dw, /* may be NULL */
                             const AzIntArr *ia_tr_dx) /* may be NULL*/
{
  if (AzDvect::isNull(v_fixed_dw)) {
    return init_const(loss_type, v_y, ia_tr_dx); 
  }

  if (loss_type == AzLoss_Logistic1 || loss_type == AzLoss_Logistic2) {
    throw new AzException("AzTrTree::init_const(weighted)", "not supported"); 
  }
  AzDvect v_wy(v_y); 
  v_wy.scale(v_fixed_dw); 
  double wy_sum = v_wy.sum(ia_tr_dx); 
  double w_sum = v_fixed_dw->sum(ia_tr_dx); 
  double const_val = wy_sum/w_sum; 
  return const_val; 
}

/*------------------------------------------------------------------*/
const AzSortedFeatArr *AzTrTree::sorted_array(int nx, 
                             const AzDataForTrTree *data) const
{
  const char *eyec = "AzTrTree::sorted_array"; 
  if (isBagging) {
    throw new AzException(eyec, "No support for bagging"); 
  }

  _checkNode(nx, "sortedFeat"); 
  if (sorted_arr == NULL) {
    throw new AzException(eyec, "no sorted_arr"); 
  }

  if (sorted_arr[nx] != NULL) {
    /*---  already exists  ---*/
    return sorted_arr[nx]; 
  }

  /*---  root  ---*/
  if (nx == root_nx) {
#if 0
    if (nodes[nx].dxs_num != data->dataNum()) {
      /*---  Do not allow sampling  ---*/
      throw new AzException(eyec, "#data mismatch"); 
    }
    return data->sorted_array(); 
#else
    if (nodes[nx].dxs_num != data->dataNum()) {
      /*---  Allow sampling  ---*/
      sorted_arr[nx] = new AzSortedFeatArr(data->sorted_array(), 
                                           nodes[nx].dxs, nodes[nx].dxs_num); 
      return sorted_arr[nx]; 
    }
    else {
      return data->sorted_array(); 
    }
#endif 
  }

  if (sorted_arr[root_nx] == NULL) {
    /*---  we need this as the base for SortedFeat_Dense  ---*/
    sorted_arr[root_nx] = new AzSortedFeatArr(data->sorted_array());     
  }
  int px = nodes[nx].parent_nx; 
  if (px < 0) {
    throw new AzException(eyec, "Not root, but no parent?!"); 
  }

  const AzSortedFeatArr *inp = sorted_arr[px]; 
  if (inp == NULL) {
    throw new AzException(eyec, "No input for separation"); 
  }

  /*---  make a new one and save it.  ---*/
  AzSortedFeatArr *base = sorted_arr[root_nx]; 

  int le_nx = nodes[px].le_nx; 
  int gt_nx = nodes[px].gt_nx; 
  if (sorted_arr[le_nx] != NULL || sorted_arr[gt_nx] != NULL) {
    throw new AzException(eyec, "one child has sorted_arr and the other doesn't?!"); 
  }
  sorted_arr[le_nx] = new AzSortedFeatArr(); 
  sorted_arr[gt_nx] = new AzSortedFeatArr(); 
  AzSortedFeatArr::separate(base, inp, 
                            nodes[le_nx].dxs, nodes[le_nx].dxs_num, 
                            nodes[gt_nx].dxs, nodes[gt_nx].dxs_num, 
                            sorted_arr[le_nx], sorted_arr[gt_nx]); 
  if (px != root_nx) { /* can't delete the one at the root as it's the base */
    delete sorted_arr[px]; sorted_arr[px] = NULL; 
  }

  return sorted_arr[nx]; 
}

